# ==============================================================================
#  Nova Programming Language - Quick PowerShell Installer
# ==============================================================================

[CmdletBinding()]
param (
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\Nova",
    [switch]$NoPath,
    [switch]$NoAssociation,
    [switch]$Uninstall
)

$ErrorActionPreference = "Stop"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "      NOVA Programming Language - Setup Script           " -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

$RepoRoot = $PSScriptRoot

# If uninstallation requested
if ($Uninstall) {
    Write-Host "`n[1/3] Removing Nova from User PATH..." -ForegroundColor Yellow
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($userPath) {
        $paths = $userPath -split ';' | Where-Object { $_ -and $_ -ne $InstallDir }
        [Environment]::SetEnvironmentVariable("Path", ($paths -join ';'), "User")
    }

    Write-Host "[2/3] Removing registry file associations..." -ForegroundColor Yellow
    Remove-Item "HKCU:\Software\Classes\.nova" -Recurse -ErrorAction SilentlyContinue
    Remove-Item "HKCU:\Software\Classes\NovaLanguageFile" -Recurse -ErrorAction SilentlyContinue
    Remove-Item "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\NovaLanguage" -Recurse -ErrorAction SilentlyContinue

    Write-Host "[3/3] Removing installation files from $InstallDir..." -ForegroundColor Yellow
    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    Write-Host "`nNova has been successfully uninstalled!" -ForegroundColor Green
    return
}

# 1. Check for nova binary
$NovaBinary = Join-Path $RepoRoot "nova.exe"
if (-not (Test-Path $NovaBinary)) {
    $NovaBinary = Join-Path $RepoRoot "build\nova.exe"
}
if (-not (Test-Path $NovaBinary)) {
    $NovaBinary = Join-Path $RepoRoot "build-release\nova.exe"
}

if (-not (Test-Path $NovaBinary)) {
    Write-Host "`n[BUILD] Building nova from source..." -ForegroundColor Yellow
    cmake -B "$RepoRoot\build" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$RepoRoot\build" --config Release
    $NovaBinary = Join-Path $RepoRoot "build\nova.exe"
}

if (-not (Test-Path $NovaBinary)) {
    Write-Error "Could not locate or build nova.exe."
}

# 2. Create destination directory
Write-Host "`n[1/4] Installing Nova to: $InstallDir" -ForegroundColor Cyan
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

# 3. Copy binaries and assets
Write-Host "[2/4] Copying interpreter binaries & standard files..." -ForegroundColor Cyan
Copy-Item $NovaBinary -Destination "$InstallDir\nova.exe" -Force

if (Test-Path "$RepoRoot\examples") {
    Copy-Item "$RepoRoot\examples" -Destination "$InstallDir\examples" -Recurse -Force
}
if (Test-Path "$RepoRoot\docs") {
    Copy-Item "$RepoRoot\docs" -Destination "$InstallDir\docs" -Recurse -Force
}
if (Test-Path "$RepoRoot\logo") {
    Copy-Item "$RepoRoot\logo" -Destination "$InstallDir\logo" -Recurse -Force
}

# 4. PATH Registration
if (-not $NoPath) {
    Write-Host "[3/4] Adding $InstallDir to User PATH..." -ForegroundColor Cyan
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($userPath -notlike "*$InstallDir*") {
        $newPath = if ($userPath) { "$userPath;$InstallDir" } else { $InstallDir }
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        $env:Path = "$env:Path;$InstallDir"
    }
}

# 5. File Association
if (-not $NoAssociation) {
    Write-Host "[4/4] Registering .nova file extension..." -ForegroundColor Cyan
    $extKey = "HKCU:\Software\Classes\.nova"
    $progKey = "HKCU:\Software\Classes\NovaLanguageFile"
    New-Item -Path $extKey -Value "NovaLanguageFile" -Force | Out-Null
    Set-ItemProperty -Path $extKey -Name "Content Type" -Value "text/plain" -Force
    New-Item -Path $progKey -Value "Nova Source File" -Force | Out-Null
    New-Item -Path "$progKey\DefaultIcon" -Value "`"$InstallDir\nova.exe`",0" -Force | Out-Null
    New-Item -Path "$progKey\shell\open\command" -Value "`"$InstallDir\nova.exe`" `"%1`"" -Force | Out-Null
}

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host "   Nova v0.2.2 installed successfully!                  " -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
Write-Host "You can now run 'nova' in your terminal or PowerShell."
Write-Host "Example: nova examples\hello.nova`n"
