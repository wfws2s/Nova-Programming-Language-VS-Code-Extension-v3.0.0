#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")

namespace fs = std::filesystem;

// Control IDs
enum ControlID {
    IDC_BTN_INSTALL = 1001,
    IDC_BTN_UNINSTALL,
    IDC_BTN_CANCEL,
    IDC_BTN_BROWSE,
    IDC_EDIT_PATH,
    IDC_CHK_PATH,
    IDC_CHK_ASSOC,
    IDC_CHK_SHORTCUT,
    IDC_CHK_EXAMPLES,
    IDC_PROGRESS,
    IDC_STATUS_TEXT,
    IDC_BTN_LAUNCH,
    IDC_BTN_DOCS
};

// Global application state
struct InstallerState {
    std::wstring installPath;
    std::wstring sourcePath;
    bool addToPath = true;
    bool associateFiles = true;
    bool createShortcut = true;
    bool copyExamples = true;
    bool isInstalled = false;
    bool isWorking = false;
    bool silent = false;
    bool uninstallMode = false;

    HWND hwndMain = nullptr;
    HWND hwndEditPath = nullptr;
    HWND hwndChkPath = nullptr;
    HWND hwndChkAssoc = nullptr;
    HWND hwndChkShortcut = nullptr;
    HWND hwndChkExamples = nullptr;
    HWND hwndProgress = nullptr;
    HWND hwndStatus = nullptr;
    HWND hwndBtnInstall = nullptr;
    HWND hwndBtnUninstall = nullptr;
    HWND hwndBtnCancel = nullptr;
    HWND hwndBtnBrowse = nullptr;
    HWND hwndBtnLaunch = nullptr;
    HWND hwndBtnDocs = nullptr;
    HFONT hFontHeading = nullptr;
    HFONT hFontNormal = nullptr;
    HFONT hFontBold = nullptr;
};

InstallerState g_state;

// Helper: Get user's default LocalAppData install directory
std::wstring GetDefaultInstallDirectory() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        fs::path p(localAppData);
        p /= L"Programs";
        p /= L"Nova";
        return p.wstring();
    }
    return L"C:\\Nova";
}

// Helper: Get current directory of this installer
std::wstring GetInstallerSourceDirectory() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    fs::path p(exePath);
    return p.parent_path().wstring();
}

// Helper: Add or Remove from User PATH
bool UpdateUserPath(const std::wstring& directory, bool add) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t buffer[32768] = {0};
    DWORD bufferSize = sizeof(buffer);
    DWORD type = REG_EXPAND_SZ;
    
    LONG result = RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)buffer, &bufferSize);
    std::wstring currentPath = (result == ERROR_SUCCESS) ? buffer : L"";

    // Split PATH by semicolon
    std::vector<std::wstring> entries;
    std::wstringstream ss(currentPath);
    std::wstring item;
    while (std::getline(ss, item, L';')) {
        if (!item.empty()) {
            // Trim whitespace
            size_t first = item.find_first_not_of(L" \t");
            size_t last = item.find_last_not_of(L" \t");
            if (first != std::wstring::npos && last != std::wstring::npos) {
                entries.push_back(item.substr(first, (last - first + 1)));
            }
        }
    }

    // Check if directory already exists
    auto it = std::find_if(entries.begin(), entries.end(), [&](const std::wstring& entry) {
        return _wcsicmp(entry.c_str(), directory.c_str()) == 0;
    });

    if (add) {
        if (it == entries.end()) {
            entries.push_back(directory);
        }
    } else {
        if (it != entries.end()) {
            entries.erase(it);
        }
    }

    // Reassemble PATH string
    std::wstring newPath;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) newPath += L";";
        newPath += entries[i];
    }

    RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)newPath.c_str(), (DWORD)((newPath.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // Broadcast environment change so all new processes pick up new PATH
    DWORD_PTR dwResult;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 3000, &dwResult);
    return true;
}

// Helper: Register/Unregister .nova file association
bool UpdateFileAssociations(const std::wstring& exePath, bool registerAssoc) {
    const wchar_t* extKey = L"Software\\Classes\\.nova";
    const wchar_t* progId = L"NovaLanguageFile";
    const wchar_t* progIdKey = L"Software\\Classes\\NovaLanguageFile";

    if (registerAssoc) {
        HKEY hKey;
        // 1. HKCU\Software\Classes\.nova
        if (RegCreateKeyExW(HKEY_CURRENT_USER, extKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)progId, (DWORD)((wcslen(progId) + 1) * sizeof(wchar_t)));
            const wchar_t* contentType = L"text/plain";
            RegSetValueExW(hKey, L"Content Type", 0, REG_SZ, (const BYTE*)contentType, (DWORD)((wcslen(contentType) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

        // 2. HKCU\Software\Classes\NovaLanguageFile
        if (RegCreateKeyExW(HKEY_CURRENT_USER, progIdKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            const wchar_t* friendlyName = L"Nova Source File";
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)friendlyName, (DWORD)((wcslen(friendlyName) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

        // 3. DefaultIcon
        std::wstring iconKey = std::wstring(progIdKey) + L"\\DefaultIcon";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, iconKey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            std::wstring iconVal = L"\"" + exePath + L"\",0";
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)iconVal.c_str(), (DWORD)((iconVal.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

        // 4. Open command
        std::wstring cmdKey = std::wstring(progIdKey) + L"\\shell\\open\\command";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, cmdKey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            std::wstring cmdVal = L"\"" + exePath + L"\" \"%1\"";
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)cmdVal.c_str(), (DWORD)((cmdVal.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    } else {
        RegDeleteTreeW(HKEY_CURRENT_USER, extKey);
        RegDeleteTreeW(HKEY_CURRENT_USER, progIdKey);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return true;
}

// Helper: Register / Unregister Windows "Add or Remove Programs"
bool UpdateUninstallRegistration(const std::wstring& targetDir, const std::wstring& exePath, const std::wstring& uninstallerPath, bool registerApp) {
    const wchar_t* uninstallRoot = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NovaLanguage";
    if (registerApp) {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, uninstallRoot, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            const wchar_t* displayName = L"Nova Programming Language Interpreter";
            const wchar_t* displayVersion = L"0.2.2";
            const wchar_t* publisher = L"Nova Project";
            const wchar_t* url = L"https://github.com/nova-lang/nova";
            std::wstring uninstallCmd = L"\"" + uninstallerPath + L"\"";

            RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (const BYTE*)displayName, (DWORD)((wcslen(displayName) + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, (const BYTE*)displayVersion, (DWORD)((wcslen(displayVersion) + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"Publisher", 0, REG_SZ, (const BYTE*)publisher, (DWORD)((wcslen(publisher) + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"HelpLink", 0, REG_SZ, (const BYTE*)url, (DWORD)((wcslen(url) + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"DisplayIcon", 0, REG_SZ, (const BYTE*)exePath.c_str(), (DWORD)((exePath.length() + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, (const BYTE*)targetDir.c_str(), (DWORD)((targetDir.length() + 1) * sizeof(wchar_t)));
            RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, (const BYTE*)uninstallCmd.c_str(), (DWORD)((uninstallCmd.length() + 1) * sizeof(wchar_t)));

            DWORD noModify = 1;
            DWORD noRepair = 1;
            RegSetValueExW(hKey, L"NoModify", 0, REG_DWORD, (const BYTE*)&noModify, sizeof(DWORD));
            RegSetValueExW(hKey, L"NoRepair", 0, REG_DWORD, (const BYTE*)&noRepair, sizeof(DWORD));

            RegCloseKey(hKey);
        }
    } else {
        RegDeleteTreeW(HKEY_CURRENT_USER, uninstallRoot);
    }
    return true;
}

// Helper: Create Desktop / Start Menu shortcut
bool CreateShortcut(const std::wstring& targetExe, const std::wstring& shortcutPath, const std::wstring& description, const std::wstring& workDir) {
    CoInitialize(NULL);
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        psl->SetPath(targetExe.c_str());
        psl->SetDescription(description.c_str());
        psl->SetWorkingDirectory(workDir.c_str());

        IPersistFile* ppf = nullptr;
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hr)) {
            ppf->Save(shortcutPath.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return SUCCEEDED(hr);
}

// Helper: Copy directory contents recursively
void CopyDirectoryRecursive(const fs::path& src, const fs::path& dst) {
    std::error_code ec;
    fs::create_directories(dst, ec);
    for (const auto& entry : fs::recursive_directory_iterator(src, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) continue;
        auto relativePath = fs::relative(entry.path(), src, ec);
        auto targetPath = dst / relativePath;
        if (entry.is_directory()) {
            fs::create_directories(targetPath, ec);
        } else if (entry.is_regular_file()) {
            fs::create_directories(targetPath.parent_path(), ec);
            fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing, ec);
        }
    }
}

// Perform Installation
bool PerformInstallation(InstallerState& state) {
    try {
        fs::path targetDir(state.installPath);
        fs::path sourceDir(state.sourcePath);
        std::error_code ec;

        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Creating installation directory...");
        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 15, 0);

        fs::create_directories(targetDir, ec);

        // 1. Find and copy nova.exe
        fs::path novaExeSource;
        std::vector<fs::path> candidates = {
            sourceDir / L"nova.exe",
            sourceDir.parent_path() / L"nova.exe",
            sourceDir.parent_path().parent_path() / L"nova.exe",
            sourceDir / L"bin" / L"nova.exe",
            sourceDir.parent_path() / L"bin" / L"nova.exe",
            sourceDir / L"build" / L"nova.exe",
            sourceDir.parent_path() / L"build" / L"nova.exe"
        };

        for (const auto& c : candidates) {
            if (fs::exists(c) && !fs::is_directory(c)) {
                novaExeSource = c;
                break;
            }
        }

        fs::path targetNovaExe = targetDir / L"nova.exe";
        if (!novaExeSource.empty()) {
            if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Copying nova.exe interpreter...");
            fs::copy_file(novaExeSource, targetNovaExe, fs::copy_options::overwrite_existing, ec);
        } else {
            if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Looking for nova interpreter binary...");
        }

        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 35, 0);

        // 2. Copy Examples if checked
        if (state.copyExamples) {
            if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Installing standard library examples and documentation...");
            std::vector<fs::path> exampleSources = {
                sourceDir / L"examples",
                sourceDir.parent_path() / L"examples",
                sourceDir.parent_path().parent_path() / L"examples"
            };
            for (const auto& es : exampleSources) {
                if (fs::exists(es) && fs::is_directory(es)) {
                    CopyDirectoryRecursive(es, targetDir / L"examples");
                    break;
                }
            }

            std::vector<fs::path> docSources = {
                sourceDir / L"docs",
                sourceDir.parent_path() / L"docs",
                sourceDir.parent_path().parent_path() / L"docs"
            };
            for (const auto& ds : docSources) {
                if (fs::exists(ds) && fs::is_directory(ds)) {
                    CopyDirectoryRecursive(ds, targetDir / L"docs");
                    break;
                }
            }

            std::vector<fs::path> logoSources = {
                sourceDir / L"logo",
                sourceDir.parent_path() / L"logo",
                sourceDir.parent_path().parent_path() / L"logo"
            };
            for (const auto& ls : logoSources) {
                if (fs::exists(ls) && fs::is_directory(ls)) {
                    CopyDirectoryRecursive(ls, targetDir / L"logo");
                    break;
                }
            }
        }

        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 55, 0);

        // 3. Create Uninstaller executable inside target directory
        wchar_t currentInstaller[MAX_PATH];
        GetModuleFileNameW(NULL, currentInstaller, MAX_PATH);
        fs::path uninstallerPath = targetDir / L"uninstall.exe";
        fs::copy_file(currentInstaller, uninstallerPath, fs::copy_options::overwrite_existing, ec);

        // 4. Update PATH
        if (state.addToPath) {
            if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Adding Nova to User PATH environment variable...");
            UpdateUserPath(targetDir.wstring(), true);
        }

        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 75, 0);

        // 5. File Associations
        if (state.associateFiles) {
            if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Registering .nova file association & icon...");
            UpdateFileAssociations(targetNovaExe.wstring(), true);
        }

        // 6. Windows Add/Remove Programs registration
        UpdateUninstallRegistration(targetDir.wstring(), targetNovaExe.wstring(), uninstallerPath.wstring(), true);

        // 7. Desktop Shortcut
        if (state.createShortcut) {
            wchar_t desktopPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
                fs::path scPath = fs::path(desktopPath) / L"Nova REPL.lnk";
                CreateShortcut(targetNovaExe.wstring(), scPath.wstring(), L"Nova Programming Language Interpreter", targetDir.wstring());
            }
        }

        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 100, 0);
        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Nova installed successfully! You can now run 'nova' in your terminal.");
        state.isInstalled = true;
        return true;
    } catch (...) {
        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Installation encountered an error.");
        return false;
    }
}

// Perform Uninstallation
bool PerformUninstallation(InstallerState& state) {
    try {
        fs::path targetDir(state.installPath);
        std::error_code ec;

        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Removing Nova from PATH environment variable...");
        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 25, 0);
        UpdateUserPath(targetDir.wstring(), false);

        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Unregistering file associations...");
        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 50, 0);
        UpdateFileAssociations(L"", false);

        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Unregistering from Windows...");
        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 75, 0);
        UpdateUninstallRegistration(L"", L"", L"", false);

        // Remove Desktop Shortcut
        wchar_t desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
            fs::path scPath = fs::path(desktopPath) / L"Nova REPL.lnk";
            fs::remove(scPath, ec);
        }

        // Clean up directory (schedule deletion on reboot if self-locked or delete files)
        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Removing files...");
        for (const auto& item : fs::directory_iterator(targetDir, ec)) {
            if (item.path().filename() != L"uninstall.exe") {
                fs::remove_all(item.path(), ec);
            }
        }

        if (state.hwndProgress) SendMessageW(state.hwndProgress, PBM_SETPOS, 100, 0);
        if (state.hwndStatus) SetWindowTextW(state.hwndStatus, L"Nova has been completely removed from your system.");
        return true;
    } catch (...) {
        return false;
    }
}

// Browse folder dialog
void BrowseFolder(HWND hwndParent) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwndParent;
    bi.lpszTitle = L"Select destination folder for Nova Language:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(g_state.hwndEditPath, path);
            g_state.installPath = path;
        }
        CoTaskMemFree(pidl);
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create modern UI controls
        g_state.hwndMain = hwnd;

        // Fonts
        g_state.hFontHeading = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_state.hFontBold = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_state.hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        // Destination Path Edit & Browse
        g_state.hwndEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.installPath.c_str(),
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 150, 420, 26, hwnd, (HMENU)IDC_EDIT_PATH, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndEditPath, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        g_state.hwndBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 460, 149, 90, 28, hwnd, (HMENU)IDC_BTN_BROWSE, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndBtnBrowse, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        // Checkboxes
        g_state.hwndChkPath = CreateWindowExW(0, L"BUTTON", L"Add Nova to User PATH (Recommended: run 'nova' from any terminal)",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 195, 520, 24, hwnd, (HMENU)IDC_CHK_PATH, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndChkPath, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);
        SendMessageW(g_state.hwndChkPath, BM_SETCHECK, BST_CHECKED, 0);

        g_state.hwndChkAssoc = CreateWindowExW(0, L"BUTTON", L"Associate .nova files with Nova interpreter and set file icon",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 225, 520, 24, hwnd, (HMENU)IDC_CHK_ASSOC, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndChkAssoc, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);
        SendMessageW(g_state.hwndChkAssoc, BM_SETCHECK, BST_CHECKED, 0);

        g_state.hwndChkExamples = CreateWindowExW(0, L"BUTTON", L"Install standard library examples and documentation files",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 255, 520, 24, hwnd, (HMENU)IDC_CHK_EXAMPLES, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndChkExamples, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);
        SendMessageW(g_state.hwndChkExamples, BM_SETCHECK, BST_CHECKED, 0);

        g_state.hwndChkShortcut = CreateWindowExW(0, L"BUTTON", L"Create Desktop shortcut for interactive Nova REPL",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 285, 520, 24, hwnd, (HMENU)IDC_CHK_SHORTCUT, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndChkShortcut, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);
        SendMessageW(g_state.hwndChkShortcut, BM_SETCHECK, BST_CHECKED, 0);

        // Progress Bar
        g_state.hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 30, 325, 520, 18, hwnd, (HMENU)IDC_PROGRESS, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndProgress, PBM_SETRANGE32, 0, 100);
        SendMessageW(g_state.hwndProgress, PBM_SETPOS, 0, 0);

        // Status Text
        g_state.hwndStatus = CreateWindowExW(0, L"STATIC", L"Ready to install Nova v0.2.2.",
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 30, 350, 520, 20, hwnd, (HMENU)IDC_STATUS_TEXT, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndStatus, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        // Action Buttons
        g_state.hwndBtnInstall = CreateWindowExW(0, L"BUTTON", L"Install Now",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 230, 385, 110, 32, hwnd, (HMENU)IDC_BTN_INSTALL, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndBtnInstall, WM_SETFONT, (WPARAM)g_state.hFontBold, TRUE);

        g_state.hwndBtnUninstall = CreateWindowExW(0, L"BUTTON", L"Uninstall",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 350, 385, 95, 32, hwnd, (HMENU)IDC_BTN_UNINSTALL, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndBtnUninstall, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        g_state.hwndBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 455, 385, 95, 32, hwnd, (HMENU)IDC_BTN_CANCEL, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndBtnCancel, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        // Finish action buttons (initially hidden)
        g_state.hwndBtnLaunch = CreateWindowExW(0, L"BUTTON", L"Launch Terminal",
            WS_CHILD | BS_PUSHBUTTON, 30, 385, 125, 32, hwnd, (HMENU)IDC_BTN_LAUNCH, GetModuleHandle(NULL), NULL);
        SendMessageW(g_state.hwndBtnLaunch, WM_SETFONT, (WPARAM)g_state.hFontNormal, TRUE);

        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Top Banner Background (Clean dark header)
        RECT bannerRect = { 0, 0, 580, 100 };
        HBRUSH hBannerBrush = CreateSolidBrush(RGB(24, 28, 36));
        FillRect(hdc, &bannerRect, hBannerBrush);
        DeleteObject(hBannerBrush);

        // Header Title
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        SelectObject(hdc, g_state.hFontHeading);
        TextOutW(hdc, 30, 18, L"NOVA Programming Language", 25);

        // Subtitle
        SetTextColor(hdc, RGB(160, 175, 195));
        SelectObject(hdc, g_state.hFontNormal);
        TextOutW(hdc, 30, 48, L"Fast, readable, and dynamic language with C++20 engine", 54);
        TextOutW(hdc, 30, 70, L"Setup Wizard • Version 0.2.2", 28);

        // Body section title
        SetTextColor(hdc, RGB(40, 40, 40));
        SelectObject(hdc, g_state.hFontBold);
        TextOutW(hdc, 30, 122, L"Installation Directory:", 23);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BTN_BROWSE:
            BrowseFolder(hwnd);
            break;

        case IDC_BTN_INSTALL: {
            if (g_state.isInstalled) {
                // Already finished, close setup
                DestroyWindow(hwnd);
                break;
            }

            wchar_t pathBuf[MAX_PATH];
            GetWindowTextW(g_state.hwndEditPath, pathBuf, MAX_PATH);
            g_state.installPath = pathBuf;

            g_state.addToPath = (SendMessageW(g_state.hwndChkPath, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_state.associateFiles = (SendMessageW(g_state.hwndChkAssoc, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_state.copyExamples = (SendMessageW(g_state.hwndChkExamples, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_state.createShortcut = (SendMessageW(g_state.hwndChkShortcut, BM_GETCHECK, 0, 0) == BST_CHECKED);

            EnableWindow(g_state.hwndBtnInstall, FALSE);
            EnableWindow(g_state.hwndBtnUninstall, FALSE);
            EnableWindow(g_state.hwndBtnBrowse, FALSE);
            EnableWindow(g_state.hwndEditPath, FALSE);

            bool success = PerformInstallation(g_state);

            if (success) {
                SetWindowTextW(g_state.hwndBtnInstall, L"Finish");
                EnableWindow(g_state.hwndBtnInstall, TRUE);
                ShowWindow(g_state.hwndBtnLaunch, SW_SHOW);
                SetWindowTextW(g_state.hwndBtnCancel, L"Close");
            } else {
                EnableWindow(g_state.hwndBtnInstall, TRUE);
                EnableWindow(g_state.hwndBtnBrowse, TRUE);
                EnableWindow(g_state.hwndEditPath, TRUE);
            }
            break;
        }

        case IDC_BTN_UNINSTALL: {
            int resp = MessageBoxW(hwnd, L"Are you sure you want to uninstall Nova and remove its PATH settings?", L"Confirm Uninstall", MB_YESNO | MB_ICONQUESTION);
            if (resp == IDYES) {
                EnableWindow(g_state.hwndBtnInstall, FALSE);
                EnableWindow(g_state.hwndBtnUninstall, FALSE);
                PerformUninstallation(g_state);
                SetWindowTextW(g_state.hwndBtnCancel, L"Close");
            }
            break;
        }

        case IDC_BTN_LAUNCH: {
            // Open terminal in install directory with nova --version
            std::wstring cmd = L"cmd.exe /k \"cd /d \"" + g_state.installPath + L"\" && nova --version && echo Type 'nova' or 'nova filename.nova' to run scripts.\"";
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi = { 0 };
            CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, g_state.installPath.c_str(), &si, &pi);
            break;
        }

        case IDC_BTN_CANCEL:
            DestroyWindow(hwnd);
            break;
        }
        break;
    }

    case WM_DESTROY:
        if (g_state.hFontHeading) DeleteObject(g_state.hFontHeading);
        if (g_state.hFontBold) DeleteObject(g_state.hFontBold);
        if (g_state.hFontNormal) DeleteObject(g_state.hFontNormal);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain Entry Point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    g_state.installPath = GetDefaultInstallDirectory();
    g_state.sourcePath = GetInstallerSourceDirectory();

    // Parse command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            if (_wcsicmp(argv[i], L"--silent") == 0 || _wcsicmp(argv[i], L"-s") == 0) {
                g_state.silent = true;
            } else if (_wcsicmp(argv[i], L"--uninstall") == 0 || _wcsicmp(argv[i], L"-u") == 0) {
                g_state.uninstallMode = true;
            } else if ((_wcsicmp(argv[i], L"--dir") == 0 || _wcsicmp(argv[i], L"-d") == 0) && i + 1 < argc) {
                g_state.installPath = argv[++i];
            }
        }
        LocalFree(argv);
    }

    // Handle silent install/uninstall modes
    if (g_state.silent) {
        if (g_state.uninstallMode) {
            PerformUninstallation(g_state);
        } else {
            PerformInstallation(g_state);
        }
        return 0;
    }

    // Register Window Class
    const wchar_t CLASS_NAME[] = L"NovaSetupWindowClass";
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    // Center window on screen
    int winWidth = 580;
    int winHeight = 470;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Nova Programming Language Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

// WinMain entry point wrapper for MinGW / GCC / Clang
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nShowCmd);
}
