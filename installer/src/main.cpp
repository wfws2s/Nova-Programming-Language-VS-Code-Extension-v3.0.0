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
#include "../resource_ids.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")

namespace fs = std::filesystem;

// Wizard Pages
enum WizardPage {
    PAGE_WELCOME = 0,
    PAGE_LICENSE,
    PAGE_DIRECTORY,
    PAGE_READY,
    PAGE_INSTALLING,
    PAGE_FINISH,
    PAGE_COUNT
};

// Control IDs
enum ControlID {
    IDC_BTN_BACK = 2001,
    IDC_BTN_NEXT,
    IDC_BTN_CANCEL,
    IDC_BTN_BROWSE,
    IDC_EDIT_LICENSE,
    IDC_CHK_ACCEPT_LICENSE,
    IDC_EDIT_PATH,
    IDC_CHK_PATH,
    IDC_CHK_ASSOC,
    IDC_CHK_VSIX,
    IDC_CHK_SHORTCUT,
    IDC_EDIT_SUMMARY,
    IDC_PROGRESS,
    IDC_STATUS_TEXT,
    IDC_CHK_LAUNCH_REPL,
    IDC_CHK_OPEN_FOLDER
};

// State
struct WizardState {
    WizardPage currentPage = PAGE_WELCOME;
    std::wstring installPath;
    
    // User Options
    bool licenseAccepted = true;
    bool addToPath = true;
    bool associateFiles = true;
    bool copyExamplesAndVsix = true;
    bool createShortcut = true;
    bool launchReplOnFinish = true;
    bool openFolderOnFinish = true;

    bool silent = false;
    bool uninstallMode = false;

    // Window & Controls
    HWND hwndMain = nullptr;
    HWND hwndBtnBack = nullptr;
    HWND hwndBtnNext = nullptr;
    HWND hwndBtnCancel = nullptr;

    // Page: Welcome
    std::vector<HWND> controlsWelcome;

    // Page: License
    std::vector<HWND> controlsLicense;
    HWND hwndEditLicense = nullptr;
    HWND hwndChkAccept = nullptr;

    // Page: Directory & Options
    std::vector<HWND> controlsDirectory;
    HWND hwndEditPath = nullptr;
    HWND hwndBtnBrowse = nullptr;
    HWND hwndChkPath = nullptr;
    HWND hwndChkAssoc = nullptr;
    HWND hwndChkExtras = nullptr;
    HWND hwndChkShortcut = nullptr;

    // Page: Ready
    std::vector<HWND> controlsReady;
    HWND hwndEditSummary = nullptr;

    // Page: Installing
    std::vector<HWND> controlsInstalling;
    HWND hwndProgress = nullptr;
    HWND hwndStatus = nullptr;

    // Page: Finish
    std::vector<HWND> controlsFinish;
    HWND hwndChkLaunchRepl = nullptr;
    HWND hwndChkOpenFolder = nullptr;

    // Fonts
    HFONT hFontTitle = nullptr;
    HFONT hFontHeading = nullptr;
    HFONT hFontBold = nullptr;
    HFONT hFontNormal = nullptr;
    HFONT hFontMonospace = nullptr;
};

WizardState g_wiz;

// Helper: Extract Embedded RCDATA Resource
bool ExtractEmbeddedPayload(int resourceId, const fs::path& outputPath) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) return false;
    DWORD size = SizeofResource(NULL, hRes);
    const void* pData = LockResource(hGlobal);
    if (!pData || size == 0) return false;

    std::error_code ec;
    fs::create_directories(outputPath.parent_path(), ec);

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(pData), size);
    return true;
}

// Helper: Read Embedded Resource as String
std::string ReadResourceString(int resourceId) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!hRes) return "";
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) return "";
    DWORD size = SizeofResource(NULL, hRes);
    const void* pData = LockResource(hGlobal);
    if (!pData || size == 0) return "";
    return std::string(reinterpret_cast<const char*>(pData), size);
}

// Helper: Default Install Path
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

// Helper: Update PATH in HKCU\Environment
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

    std::vector<std::wstring> entries;
    std::wstringstream ss(currentPath);
    std::wstring item;
    while (std::getline(ss, item, L';')) {
        if (!item.empty()) {
            size_t first = item.find_first_not_of(L" \t");
            size_t last = item.find_last_not_of(L" \t");
            if (first != std::wstring::npos && last != std::wstring::npos) {
                entries.push_back(item.substr(first, (last - first + 1)));
            }
        }
    }

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

    std::wstring newPath;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) newPath += L";";
        newPath += entries[i];
    }

    RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)newPath.c_str(), (DWORD)((newPath.length() + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    DWORD_PTR dwResult;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 3000, &dwResult);
    return true;
}

// Helper: Register/Unregister File Associations (.nova)
bool UpdateFileAssociations(const std::wstring& exePath, bool registerAssoc) {
    const wchar_t* extKey = L"Software\\Classes\\.nova";
    const wchar_t* progId = L"NovaLanguageFile";
    const wchar_t* progIdKey = L"Software\\Classes\\NovaLanguageFile";

    if (registerAssoc) {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, extKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)progId, (DWORD)((wcslen(progId) + 1) * sizeof(wchar_t)));
            const wchar_t* contentType = L"text/plain";
            RegSetValueExW(hKey, L"Content Type", 0, REG_SZ, (const BYTE*)contentType, (DWORD)((wcslen(contentType) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

        if (RegCreateKeyExW(HKEY_CURRENT_USER, progIdKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            const wchar_t* friendlyName = L"Nova Source File";
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)friendlyName, (DWORD)((wcslen(friendlyName) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

        std::wstring iconKey = std::wstring(progIdKey) + L"\\DefaultIcon";
        if (RegCreateKeyExW(HKEY_CURRENT_USER, iconKey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            std::wstring iconVal = L"\"" + exePath + L"\",0";
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)iconVal.c_str(), (DWORD)((iconVal.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }

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

// Helper: Windows Add/Remove Programs
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

// Helper: Desktop Shortcut
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

// Helper: Update Wizard Controls Visibility
void SwitchPage(WizardPage newPage) {
    auto setGroupVisible = [](const std::vector<HWND>& group, bool visible) {
        int cmd = visible ? SW_SHOW : SW_HIDE;
        for (HWND h : group) {
            ShowWindow(h, cmd);
        }
    };

    // Hide all
    setGroupVisible(g_wiz.controlsWelcome, false);
    setGroupVisible(g_wiz.controlsLicense, false);
    setGroupVisible(g_wiz.controlsDirectory, false);
    setGroupVisible(g_wiz.controlsReady, false);
    setGroupVisible(g_wiz.controlsInstalling, false);
    setGroupVisible(g_wiz.controlsFinish, false);

    g_wiz.currentPage = newPage;

    // Button states
    switch (newPage) {
    case PAGE_WELCOME:
        setGroupVisible(g_wiz.controlsWelcome, true);
        EnableWindow(g_wiz.hwndBtnBack, FALSE);
        EnableWindow(g_wiz.hwndBtnNext, TRUE);
        SetWindowTextW(g_wiz.hwndBtnNext, L"Next >");
        EnableWindow(g_wiz.hwndBtnCancel, TRUE);
        break;

    case PAGE_LICENSE:
        setGroupVisible(g_wiz.controlsLicense, true);
        EnableWindow(g_wiz.hwndBtnBack, TRUE);
        EnableWindow(g_wiz.hwndBtnNext, g_wiz.licenseAccepted ? TRUE : FALSE);
        SetWindowTextW(g_wiz.hwndBtnNext, L"Next >");
        EnableWindow(g_wiz.hwndBtnCancel, TRUE);
        break;

    case PAGE_DIRECTORY:
        setGroupVisible(g_wiz.controlsDirectory, true);
        EnableWindow(g_wiz.hwndBtnBack, TRUE);
        EnableWindow(g_wiz.hwndBtnNext, TRUE);
        SetWindowTextW(g_wiz.hwndBtnNext, L"Next >");
        EnableWindow(g_wiz.hwndBtnCancel, TRUE);
        break;

    case PAGE_READY: {
        setGroupVisible(g_wiz.controlsReady, true);
        EnableWindow(g_wiz.hwndBtnBack, TRUE);
        EnableWindow(g_wiz.hwndBtnNext, TRUE);
        SetWindowTextW(g_wiz.hwndBtnNext, L"Install");
        EnableWindow(g_wiz.hwndBtnCancel, TRUE);

        // Update Summary Text
        std::wstring summary = L"Destination location:\r\n  " + g_wiz.installPath + L"\r\n\r\n"
            L"Components to be installed:\r\n"
            L"  • Nova CLI Interpreter (nova.exe)\r\n"
            L"  • VS Code Extension (nova-lang-0.4.0.vsix)\r\n"
            L"  • Standard Examples & Documentation\r\n\r\n"
            L"Selected setup tasks:\r\n"
            L"  • Add to User PATH: " + (g_wiz.addToPath ? L"Yes" : L"No") + L"\r\n"
            L"  • File Association (.nova): " + (g_wiz.associateFiles ? L"Yes" : L"No") + L"\r\n"
            L"  • Desktop Shortcut: " + (g_wiz.createShortcut ? L"Yes" : L"No") + L"\r\n";
        SetWindowTextW(g_wiz.hwndEditSummary, summary.c_str());
        break;
    }

    case PAGE_INSTALLING:
        setGroupVisible(g_wiz.controlsInstalling, true);
        EnableWindow(g_wiz.hwndBtnBack, FALSE);
        EnableWindow(g_wiz.hwndBtnNext, FALSE);
        EnableWindow(g_wiz.hwndBtnCancel, FALSE);
        break;

    case PAGE_FINISH:
        setGroupVisible(g_wiz.controlsFinish, true);
        EnableWindow(g_wiz.hwndBtnBack, FALSE);
        EnableWindow(g_wiz.hwndBtnNext, TRUE);
        SetWindowTextW(g_wiz.hwndBtnNext, L"Finish");
        ShowWindow(g_wiz.hwndBtnCancel, SW_HIDE);
        break;

    default:
        break;
    }

    InvalidateRect(g_wiz.hwndMain, NULL, TRUE);
}

// Perform Installation of Embedded Files
bool DoInstallation() {
    SwitchPage(PAGE_INSTALLING);
    fs::path targetDir(g_wiz.installPath);
    std::error_code ec;

    // 1. Create directory
    SetWindowTextW(g_wiz.hwndStatus, L"Creating installation folder...");
    SendMessageW(g_wiz.hwndProgress, PBM_SETPOS, 15, 0);
    fs::create_directories(targetDir, ec);
    fs::create_directories(targetDir / L"examples", ec);
    fs::create_directories(targetDir / L"extension", ec);
    fs::create_directories(targetDir / L"logo", ec);

    // 2. Extract nova.exe
    SetWindowTextW(g_wiz.hwndStatus, L"Extracting Nova interpreter (nova.exe)...");
    SendMessageW(g_wiz.hwndProgress, PBM_SETPOS, 35, 0);
    fs::path novaExe = targetDir / L"nova.exe";
    ExtractEmbeddedPayload(IDR_NOVA_EXE, novaExe);

    // 3. Extract VS Code extension & Examples
    if (g_wiz.copyExamplesAndVsix) {
        SetWindowTextW(g_wiz.hwndStatus, L"Extracting VS Code extension (nova-lang-0.4.0.vsix)...");
        ExtractEmbeddedPayload(IDR_NOVA_VSIX, targetDir / L"extension" / L"nova-lang-0.4.0.vsix");
        // Also place a convenient copy in root install dir
        ExtractEmbeddedPayload(IDR_NOVA_VSIX, targetDir / L"nova-lang-0.4.0.vsix");

        SetWindowTextW(g_wiz.hwndStatus, L"Extracting standard examples and documentation...");
        ExtractEmbeddedPayload(IDR_EX_HELLO, targetDir / L"examples" / L"hello.nova");
        ExtractEmbeddedPayload(IDR_EX_OOP, targetDir / L"examples" / L"oop_and_features_demo.nova");
        ExtractEmbeddedPayload(IDR_EX_FIB, targetDir / L"examples" / L"fibonacci.nova");
        ExtractEmbeddedPayload(IDR_EX_ARRAYS, targetDir / L"examples" / L"arrays.nova");
        ExtractEmbeddedPayload(IDR_EX_MATH, targetDir / L"examples" / L"math_helper.nova");
        ExtractEmbeddedPayload(IDR_APP_ICO_FILE, targetDir / L"logo" / L"R-removebg-preview.ico");
        ExtractEmbeddedPayload(IDR_README_MD, targetDir / L"README.md");
        ExtractEmbeddedPayload(IDR_LICENSE_TXT, targetDir / L"LICENSE");
    }

    SendMessageW(g_wiz.hwndProgress, PBM_SETPOS, 60, 0);

    // 4. Create Uninstaller
    wchar_t currentInstaller[MAX_PATH];
    GetModuleFileNameW(NULL, currentInstaller, MAX_PATH);
    fs::path uninstallerPath = targetDir / L"uninstall.exe";
    fs::copy_file(currentInstaller, uninstallerPath, fs::copy_options::overwrite_existing, ec);

    // 5. Update PATH
    if (g_wiz.addToPath) {
        SetWindowTextW(g_wiz.hwndStatus, L"Configuring User PATH environment variable...");
        UpdateUserPath(targetDir.wstring(), true);
    }
    SendMessageW(g_wiz.hwndProgress, PBM_SETPOS, 80, 0);

    // 6. File Association
    if (g_wiz.associateFiles) {
        SetWindowTextW(g_wiz.hwndStatus, L"Registering .nova file association & icon...");
        UpdateFileAssociations(novaExe.wstring(), true);
    }

    // 7. Windows Registry
    UpdateUninstallRegistration(targetDir.wstring(), novaExe.wstring(), uninstallerPath.wstring(), true);

    // 8. Desktop Shortcut
    if (g_wiz.createShortcut) {
        wchar_t desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
            fs::path scPath = fs::path(desktopPath) / L"Nova REPL.lnk";
            CreateShortcut(novaExe.wstring(), scPath.wstring(), L"Nova Programming Language Interpreter", targetDir.wstring());
        }
    }

    SendMessageW(g_wiz.hwndProgress, PBM_SETPOS, 100, 0);
    SetWindowTextW(g_wiz.hwndStatus, L"Setup completed successfully!");

    Sleep(500);
    SwitchPage(PAGE_FINISH);
    return true;
}

// Perform Uninstall
bool DoUninstallation() {
    fs::path targetDir(g_wiz.installPath);
    std::error_code ec;
    UpdateUserPath(targetDir.wstring(), false);
    UpdateFileAssociations(L"", false);
    UpdateUninstallRegistration(L"", L"", L"", false);

    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        fs::path scPath = fs::path(desktopPath) / L"Nova REPL.lnk";
        fs::remove(scPath, ec);
    }

    for (const auto& item : fs::directory_iterator(targetDir, ec)) {
        if (item.path().filename() != L"uninstall.exe") {
            fs::remove_all(item.path(), ec);
        }
    }
    return true;
}

// Browse folder dialog
void BrowseFolder(HWND hwndParent) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwndParent;
    bi.lpszTitle = L"Select destination folder for NOVA:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(g_wiz.hwndEditPath, path);
            g_wiz.installPath = path;
        }
        CoTaskMemFree(pidl);
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_wiz.hwndMain = hwnd;

        // Fonts
        g_wiz.hFontTitle = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_wiz.hFontHeading = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_wiz.hFontBold = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_wiz.hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_wiz.hFontMonospace = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

        // Bottom Navigation Buttons
        g_wiz.hwndBtnBack = CreateWindowExW(0, L"BUTTON", L"< Back", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 420, 85, 30, hwnd, (HMENU)IDC_BTN_BACK, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndBtnBack, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);

        g_wiz.hwndBtnNext = CreateWindowExW(0, L"BUTTON", L"Next >", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 422, 420, 90, 30, hwnd, (HMENU)IDC_BTN_NEXT, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndBtnNext, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);

        g_wiz.hwndBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 519, 420, 85, 30, hwnd, (HMENU)IDC_BTN_CANCEL, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndBtnCancel, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);

        // ==================== PAGE 0: WELCOME ====================
        HWND hW1 = CreateWindowExW(0, L"STATIC", L"Welcome to the NOVA Setup Wizard", WS_CHILD | SS_LEFT, 35, 105, 550, 25, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hW1, WM_SETFONT, (WPARAM)g_wiz.hFontTitle, TRUE);
        g_wiz.controlsWelcome.push_back(hW1);

        HWND hW2 = CreateWindowExW(0, L"STATIC",
            L"This wizard will install NOVA Programming Language Interpreter v0.2.2 on your computer.\r\n\r\n"
            L"NOVA is a modern, readable, dynamically-typed programming language engineered with C++20 engine.\r\n\r\n"
            L"What this setup includes:\r\n"
            L"  • Standalone Nova CLI Interpreter (nova.exe)\r\n"
            L"  • VS Code Syntax & Snippets Extension (nova-lang-0.4.0.vsix)\r\n"
            L"  • Standard Library Modules (math, random, string, list, dict)\r\n"
            L"  • Sample Projects & Documentation\r\n"
            L"  • Automatic PATH Environment Setup\r\n\r\n"
            L"Click Next to continue with the setup.",
            WS_CHILD | SS_LEFT, 35, 140, 550, 240, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hW2, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsWelcome.push_back(hW2);

        // ==================== PAGE 1: LICENSE ====================
        HWND hL1 = CreateWindowExW(0, L"STATIC", L"Please review the license terms before installing NOVA:", WS_CHILD | SS_LEFT, 35, 95, 550, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hL1, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);
        g_wiz.controlsLicense.push_back(hL1);

        std::string licStr = ReadResourceString(IDR_LICENSE_TXT);
        if (licStr.empty()) {
            licStr = "MIT License\r\n\r\nCopyright (c) 2026 Nova Programming Language Project\r\n\r\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files, to deal in the Software without restriction.";
        }
        std::wstring wLic(licStr.begin(), licStr.end());

        g_wiz.hwndEditLicense = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wLic.c_str(),
            WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 35, 120, 550, 210, hwnd, (HMENU)IDC_EDIT_LICENSE, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndEditLicense, WM_SETFONT, (WPARAM)g_wiz.hFontMonospace, TRUE);
        g_wiz.controlsLicense.push_back(g_wiz.hwndEditLicense);

        g_wiz.hwndChkAccept = CreateWindowExW(0, L"BUTTON", L"I accept the terms in the License Agreement / ฉันยอมรับข้อตกลงและเงื่อนไขการใช้งาน",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 345, 550, 25, hwnd, (HMENU)IDC_CHK_ACCEPT_LICENSE, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkAccept, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);
        SendMessageW(g_wiz.hwndChkAccept, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsLicense.push_back(g_wiz.hwndChkAccept);

        // ==================== PAGE 2: DIRECTORY & OPTIONS ====================
        HWND hD1 = CreateWindowExW(0, L"STATIC", L"Destination Folder / ตำแหน่งติดตั้ง:", WS_CHILD | SS_LEFT, 35, 95, 550, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hD1, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);
        g_wiz.controlsDirectory.push_back(hD1);

        g_wiz.hwndEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_wiz.installPath.c_str(),
            WS_CHILD | ES_AUTOHSCROLL, 35, 120, 445, 26, hwnd, (HMENU)IDC_EDIT_PATH, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndEditPath, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndEditPath);

        g_wiz.hwndBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse...",
            WS_CHILD | BS_PUSHBUTTON, 490, 119, 95, 28, hwnd, (HMENU)IDC_BTN_BROWSE, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndBtnBrowse, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndBtnBrowse);

        HWND hD2 = CreateWindowExW(0, L"STATIC", L"Setup Options / ตัวเลือกเสริม:", WS_CHILD | SS_LEFT, 35, 165, 550, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hD2, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);
        g_wiz.controlsDirectory.push_back(hD2);

        g_wiz.hwndChkPath = CreateWindowExW(0, L"BUTTON", L"Add Nova to User PATH (แนะนำ: สามารถพิมพ์ 'nova' ใน Terminal ได้ทันที)",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 195, 550, 24, hwnd, (HMENU)IDC_CHK_PATH, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkPath, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        SendMessageW(g_wiz.hwndChkPath, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndChkPath);

        g_wiz.hwndChkAssoc = CreateWindowExW(0, L"BUTTON", L"Associate .nova files & set file icons (ผูกนามสกุล .nova และตั้งไอคอน)",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 225, 550, 24, hwnd, (HMENU)IDC_CHK_ASSOC, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkAssoc, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        SendMessageW(g_wiz.hwndChkAssoc, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndChkAssoc);

        g_wiz.hwndChkExtras = CreateWindowExW(0, L"BUTTON", L"Install VS Code Extension (.vsix) & sample projects",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 255, 550, 24, hwnd, (HMENU)IDC_CHK_VSIX, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkExtras, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        SendMessageW(g_wiz.hwndChkExtras, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndChkExtras);

        g_wiz.hwndChkShortcut = CreateWindowExW(0, L"BUTTON", L"Create Desktop shortcut for interactive Nova REPL",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 285, 550, 24, hwnd, (HMENU)IDC_CHK_SHORTCUT, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkShortcut, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        SendMessageW(g_wiz.hwndChkShortcut, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsDirectory.push_back(g_wiz.hwndChkShortcut);

        // ==================== PAGE 3: READY ====================
        HWND hR1 = CreateWindowExW(0, L"STATIC", L"Ready to Install NOVA", WS_CHILD | SS_LEFT, 35, 95, 550, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hR1, WM_SETFONT, (WPARAM)g_wiz.hFontTitle, TRUE);
        g_wiz.controlsReady.push_back(hR1);

        g_wiz.hwndEditSummary = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 35, 125, 550, 240, hwnd, (HMENU)IDC_EDIT_SUMMARY, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndEditSummary, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsReady.push_back(g_wiz.hwndEditSummary);

        // ==================== PAGE 4: INSTALLING ====================
        HWND hI1 = CreateWindowExW(0, L"STATIC", L"Installing NOVA Programming Language...", WS_CHILD | SS_LEFT, 35, 120, 550, 25, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hI1, WM_SETFONT, (WPARAM)g_wiz.hFontTitle, TRUE);
        g_wiz.controlsInstalling.push_back(hI1);

        g_wiz.hwndProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | PBS_SMOOTH, 35, 160, 550, 22, hwnd, (HMENU)IDC_PROGRESS, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndProgress, PBM_SETRANGE32, 0, 100);
        g_wiz.controlsInstalling.push_back(g_wiz.hwndProgress);

        g_wiz.hwndStatus = CreateWindowExW(0, L"STATIC", L"Extracting files...", WS_CHILD | SS_LEFTNOWORDWRAP, 35, 195, 550, 25, hwnd, (HMENU)IDC_STATUS_TEXT, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndStatus, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsInstalling.push_back(g_wiz.hwndStatus);

        // ==================== PAGE 5: FINISH ====================
        HWND hF1 = CreateWindowExW(0, L"STATIC", L"NOVA Installation Complete! / ติดตั้งเสร็จสมบูรณ์", WS_CHILD | SS_LEFT, 35, 105, 550, 25, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hF1, WM_SETFONT, (WPARAM)g_wiz.hFontTitle, TRUE);
        g_wiz.controlsFinish.push_back(hF1);

        HWND hF2 = CreateWindowExW(0, L"STATIC",
            L"NOVA Programming Language v0.2.2 has been installed on your computer.\r\n\r\n"
            L"You can now create and run .nova scripts immediately:\r\n"
            L"  • Run 'nova script.nova' in any Terminal or PowerShell\r\n"
            L"  • Start interactive REPL mode by simply typing 'nova'\r\n\r\n"
            L"VS Code Extension:\r\n"
            L"  • 'nova-lang-0.4.0.vsix' is ready in your installation folder.\r\n"
            L"  • To install in VS Code: Press Ctrl+Shift+P -> 'Install from VSIX...' and pick the file.\r\n",
            WS_CHILD | SS_LEFT, 35, 140, 550, 170, hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hF2, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        g_wiz.controlsFinish.push_back(hF2);

        g_wiz.hwndChkLaunchRepl = CreateWindowExW(0, L"BUTTON", L"Launch Nova Terminal / REPL now (เปิดหน้าต่าง Terminal ทดสอบทันที)",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 315, 550, 24, hwnd, (HMENU)IDC_CHK_LAUNCH_REPL, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkLaunchRepl, WM_SETFONT, (WPARAM)g_wiz.hFontBold, TRUE);
        SendMessageW(g_wiz.hwndChkLaunchRepl, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsFinish.push_back(g_wiz.hwndChkLaunchRepl);

        g_wiz.hwndChkOpenFolder = CreateWindowExW(0, L"BUTTON", L"Open Nova installation folder (เปิดโฟลเดอร์ติดตั้งเพื่อดูไฟล์และ .vsix)",
            WS_CHILD | BS_AUTOCHECKBOX, 35, 345, 550, 24, hwnd, (HMENU)IDC_CHK_OPEN_FOLDER, GetModuleHandle(NULL), NULL);
        SendMessageW(g_wiz.hwndChkOpenFolder, WM_SETFONT, (WPARAM)g_wiz.hFontNormal, TRUE);
        SendMessageW(g_wiz.hwndChkOpenFolder, BM_SETCHECK, BST_CHECKED, 0);
        g_wiz.controlsFinish.push_back(g_wiz.hwndChkOpenFolder);

        // Show initial page
        SwitchPage(PAGE_WELCOME);
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Header Background Banner
        RECT bannerRect = { 0, 0, 640, 75 };
        HBRUSH hBannerBrush = CreateSolidBrush(RGB(24, 28, 36));
        FillRect(hdc, &bannerRect, hBannerBrush);
        DeleteObject(hBannerBrush);

        // Header Divider Line
        RECT lineRect = { 0, 74, 640, 76 };
        HBRUSH hLineBrush = CreateSolidBrush(RGB(50, 58, 70));
        FillRect(hdc, &lineRect, hLineBrush);
        DeleteObject(hLineBrush);

        // Header Title
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        SelectObject(hdc, g_wiz.hFontHeading);
        TextOutW(hdc, 25, 14, L"NOVA Programming Language Setup", 31);

        // Header Subtitle
        SetTextColor(hdc, RGB(165, 180, 200));
        SelectObject(hdc, g_wiz.hFontNormal);
        std::wstring pageSubtitle = L"Setup Wizard • Version 0.2.2 (x64 Native Engine)";
        if (g_wiz.currentPage == PAGE_LICENSE) pageSubtitle = L"License Agreement & Terms of Use";
        else if (g_wiz.currentPage == PAGE_DIRECTORY) pageSubtitle = L"Choose Destination Location & System Options";
        else if (g_wiz.currentPage == PAGE_READY) pageSubtitle = L"Review Settings and Start Installation";
        else if (g_wiz.currentPage == PAGE_INSTALLING) pageSubtitle = L"Extracting components and configuring environment...";
        else if (g_wiz.currentPage == PAGE_FINISH) pageSubtitle = L"Installation Complete";
        TextOutW(hdc, 25, 38, pageSubtitle.c_str(), (int)pageSubtitle.length());

        // Bottom Divider Line
        RECT botLineRect = { 0, 405, 640, 407 };
        HBRUSH hBotLineBrush = CreateSolidBrush(RGB(215, 220, 228));
        FillRect(hdc, &botLineRect, hBotLineBrush);
        DeleteObject(hBotLineBrush);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_CHK_ACCEPT_LICENSE: {
            g_wiz.licenseAccepted = (SendMessageW(g_wiz.hwndChkAccept, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(g_wiz.hwndBtnNext, g_wiz.licenseAccepted ? TRUE : FALSE);
            break;
        }

        case IDC_BTN_BROWSE:
            BrowseFolder(hwnd);
            break;

        case IDC_BTN_BACK:
            if (g_wiz.currentPage > PAGE_WELCOME && g_wiz.currentPage < PAGE_INSTALLING) {
                SwitchPage((WizardPage)(g_wiz.currentPage - 1));
            }
            break;

        case IDC_BTN_NEXT: {
            if (g_wiz.currentPage == PAGE_WELCOME) {
                SwitchPage(PAGE_LICENSE);
            } else if (g_wiz.currentPage == PAGE_LICENSE) {
                SwitchPage(PAGE_DIRECTORY);
            } else if (g_wiz.currentPage == PAGE_DIRECTORY) {
                wchar_t pathBuf[MAX_PATH];
                GetWindowTextW(g_wiz.hwndEditPath, pathBuf, MAX_PATH);
                g_wiz.installPath = pathBuf;
                g_wiz.addToPath = (SendMessageW(g_wiz.hwndChkPath, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_wiz.associateFiles = (SendMessageW(g_wiz.hwndChkAssoc, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_wiz.copyExamplesAndVsix = (SendMessageW(g_wiz.hwndChkExtras, BM_GETCHECK, 0, 0) == BST_CHECKED);
                g_wiz.createShortcut = (SendMessageW(g_wiz.hwndChkShortcut, BM_GETCHECK, 0, 0) == BST_CHECKED);
                SwitchPage(PAGE_READY);
            } else if (g_wiz.currentPage == PAGE_READY) {
                DoInstallation();
            } else if (g_wiz.currentPage == PAGE_FINISH) {
                bool launchRepl = (SendMessageW(g_wiz.hwndChkLaunchRepl, BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool openFolder = (SendMessageW(g_wiz.hwndChkOpenFolder, BM_GETCHECK, 0, 0) == BST_CHECKED);

                if (launchRepl) {
                    std::wstring cmd = L"cmd.exe /k \"cd /d \"" + g_wiz.installPath + L"\" && nova --version && echo Welcome to NOVA REPL! Type 'nova' or 'nova filename.nova' to run scripts.\"";
                    STARTUPINFOW si = { sizeof(si) };
                    PROCESS_INFORMATION pi = { 0 };
                    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, g_wiz.installPath.c_str(), &si, &pi);
                }

                if (openFolder) {
                    ShellExecuteW(NULL, L"open", g_wiz.installPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }

                DestroyWindow(hwnd);
            }
            break;
        }

        case IDC_BTN_CANCEL:
            if (g_wiz.currentPage == PAGE_FINISH) {
                DestroyWindow(hwnd);
            } else {
                int resp = MessageBoxW(hwnd, L"Are you sure you want to exit the setup?", L"Exit Setup", MB_YESNO | MB_ICONQUESTION);
                if (resp == IDYES) {
                    DestroyWindow(hwnd);
                }
            }
            break;
        }
        break;
    }

    case WM_DESTROY:
        if (g_wiz.hFontTitle) DeleteObject(g_wiz.hFontTitle);
        if (g_wiz.hFontHeading) DeleteObject(g_wiz.hFontHeading);
        if (g_wiz.hFontBold) DeleteObject(g_wiz.hFontBold);
        if (g_wiz.hFontNormal) DeleteObject(g_wiz.hFontNormal);
        if (g_wiz.hFontMonospace) DeleteObject(g_wiz.hFontMonospace);
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

    g_wiz.installPath = GetDefaultInstallDirectory();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            if (_wcsicmp(argv[i], L"--silent") == 0 || _wcsicmp(argv[i], L"-s") == 0) {
                g_wiz.silent = true;
            } else if (_wcsicmp(argv[i], L"--uninstall") == 0 || _wcsicmp(argv[i], L"-u") == 0) {
                g_wiz.uninstallMode = true;
            } else if ((_wcsicmp(argv[i], L"--dir") == 0 || _wcsicmp(argv[i], L"-d") == 0) && i + 1 < argc) {
                g_wiz.installPath = argv[++i];
            }
        }
        LocalFree(argv);
    }

    if (g_wiz.silent) {
        if (g_wiz.uninstallMode) {
            DoUninstallation();
        } else {
            DoInstallation();
        }
        return 0;
    }

    const wchar_t CLASS_NAME[] = L"NovaWizardSetupWindowClass";
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    int winWidth = 640;
    int winHeight = 505;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winWidth) / 2;
    int posY = (screenH - winHeight) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"NOVA Programming Language Setup Wizard",
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nShowCmd) {
    (void)hPrevInstance;
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nShowCmd);
}
