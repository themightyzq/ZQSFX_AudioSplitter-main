@echo off
setlocal EnableDelayedExpansion

REM Change to script directory
cd /d "%~dp0"

set "APP_NAME=ZQ SFX Audio Splitter"
set "DIST_DIR=dist"
set "BUILD_DIR=build"
set "PYTHON_CMD=python"

REM Create and activate virtual environment
if not exist "venv\Scripts\activate.bat" (
    echo Creating virtual environment...
    %PYTHON_CMD% -m venv venv
)
call venv\Scripts\activate

echo Starting build process...

REM Step 1: Terminate running instances
echo Terminating any running instances of "%APP_NAME%"...
taskkill /FI "WINDOWTITLE eq %APP_NAME%" /F >nul 2>&1 || echo No running instances found.

REM Step 2: Clean previous build artifacts
echo Cleaning previous build directories...
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
for %%F in (*.spec) do (
    echo Removing existing spec file: %%F
    del "%%F"
)

REM Step 3: Verify FFmpeg binaries
echo Verifying presence of FFmpeg binaries...
if not exist "ffmpeg\ffmpeg.exe" (
    echo Error: ffmpeg.exe not found in the ffmpeg directory.
    exit /b 1
)
if not exist "ffmpeg\ffprobe.exe" (
    echo Error: ffprobe.exe not found in the ffmpeg directory.
    exit /b 1
)
echo FFmpeg binaries found.

REM Step 4: Install dependencies
echo Installing dependencies...
python -m pip install --upgrade pip
python -m pip install pyinstaller tkinterdnd2-universal tk pydub numpy

REM Step 5: Get tkinterdnd2 module path (note: not tkinterdnd2_universal)
for /f "tokens=*" %%i in ('python -c "import tkinterdnd2; import os; print(os.path.dirname(tkinterdnd2.__file__))"') do set "TKDND_PATH=%%i"
if "%TKDND_PATH%"=="" (
    echo Error: tkinterdnd2 not found.
    exit /b 1
)
echo Using tkinterdnd2 at: %TKDND_PATH%

REM Step 6: Run PyInstaller with proper --add-data usage
pyinstaller --onefile --name "%APP_NAME%" audio_splitter_gui.py ^
    --icon "icon.ico" ^
    --add-binary "ffmpeg\ffmpeg.exe;ffmpeg" ^
    --add-binary "ffmpeg\ffprobe.exe;ffmpeg" ^
    --add-data "%TKDND_PATH%;tkinterdnd2" ^
    --hidden-import=tkinterdnd2 ^
    --hidden-import=tkinter ^
    --hidden-import=tkinter.ttk ^
    --hidden-import=pydub.utils ^
    --hidden-import=pydub ^
    --hidden-import=numpy ^
    --log-level=DEBUG

if errorlevel 1 (
    echo Build process failed.
    exit /b 1
)

echo Build process completed successfully.
endlocal
pause