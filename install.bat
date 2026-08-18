@echo off
setlocal
set SCRIPT_DIR=%~dp0

echo ========================================================
echo       NOVA Programming Language - Setup Script
echo ========================================================

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%install.ps1" %*

if %ERRORLEVEL% equ 0 (
    echo.
    echo Setup finished successfully.
) else (
    echo.
    echo Setup encountered an error.
)
pause
