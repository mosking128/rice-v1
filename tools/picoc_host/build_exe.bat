@echo off
setlocal

cd /d "%~dp0"

set SRC_DIR=%cd%\src
set BUILD_DIR=%cd%\build
set DIST_DIR=%cd%\dist
set RELEASE_DIR=%cd%\release

python -m PyInstaller --version >nul 2>&1
if errorlevel 1 (
    if exist wheelhouse (
        echo Installing PyInstaller from local wheelhouse...
        python -m pip install --no-index --find-links=wheelhouse pyinstaller
    )
    python -m PyInstaller --version >nul 2>&1
    if errorlevel 1 (
        echo.
        echo PyInstaller is not installed.
        echo Please install it first:
        echo   pip install pyinstaller
        echo.
        pause
        exit /b 1
    )
)

python -m PyInstaller ^
  --noconfirm ^
  --clean ^
  --windowed ^
  --name MCUStudio ^
  --specpath "%BUILD_DIR%" ^
  --workpath "%BUILD_DIR%" ^
  --distpath "%DIST_DIR%" ^
  "%SRC_DIR%\app.py"

if errorlevel 1 (
    echo.
    echo Build failed.
    echo Please make sure dependencies are installed:
    echo   pip install -r src\requirements.txt
    echo   pip install pyinstaller
    echo.
    pause
    exit /b 1
)

if exist "%RELEASE_DIR%" rd /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"
xcopy /e /i /y "%DIST_DIR%\MCUStudio" "%RELEASE_DIR%\MCUStudio" >nul
copy "%SRC_DIR%\README.md" "%RELEASE_DIR%\README.txt" >nul
copy "%SRC_DIR%\README.md" "%RELEASE_DIR%\MCUStudio\README.txt" >nul

echo.
echo Build finished.
echo Source folder: %SRC_DIR%
echo EXE folder: %DIST_DIR%\MCUStudio
echo Release folder: %RELEASE_DIR%\MCUStudio
echo.
pause

endlocal
