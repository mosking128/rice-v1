@echo off
setlocal

cd /d "%~dp0"

if not exist "release\MCUStudio\MCUStudio.exe" (
    echo.
    echo release\MCUStudio not found. Please run build_exe.bat first.
    echo.
    pause
    exit /b 1
)

where iscc >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
        set ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe
    ) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
        set ISCC=C:\Program Files\Inno Setup 6\ISCC.exe
    ) else if exist "D:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
        set ISCC=D:\Program Files (x86)\Inno Setup 6\ISCC.exe
    ) else if exist "D:\Program Files\Inno Setup 6\ISCC.exe" (
        set ISCC=D:\Program Files\Inno Setup 6\ISCC.exe
    ) else (
        echo.
        echo Inno Setup not found.
        echo Please install it from: https://jrsoftware.org/isinfo.php
        echo.
        pause
        exit /b 1
    )
) else (
    set ISCC=iscc
)

"%ISCC%" installer.iss

if errorlevel 1 (
    echo.
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo Installer built: installer_output\MCUStudio_Setup_1.0.0.exe
echo.
pause

endlocal
