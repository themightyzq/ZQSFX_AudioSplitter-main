@echo off

echo Starting build process...

set APP_NAME=ZQ SFX Audio Splitter

REM Step 1: Terminate any running instances of the application
echo Terminating any running instances of "%APP_NAME%"...
taskkill /IM "%APP_NAME%.exe" /F >nul 2>&1 || echo No running instances found.

REM Step 2: Clean previous build directories
echo Cleaning previous build directories...
if exist dist (
    rmdir /s /q dist
) else (
    echo No 'dist' directory found.
)

if exist build (
    rmdir /s /q build
) else (
    echo No 'build' directory found.
)

REM Step 3: Remove any existing spec files
echo Removing existing spec files...
del /f *.spec 2>nul || echo No spec files found.

REM Step 4: Verify FFmpeg Binaries
echo Verifying presence of FFmpeg binaries...
if not exist "ffmpeg\ffmpeg.exe" (
    echo Error: 'ffmpeg.exe' not found in the 'ffmpeg' directory.
    echo Please ensure that 'ffmpeg.exe' and 'ffprobe.exe' are placed inside the 'ffmpeg' folder.
    pause
    exit /b 1
)

if not exist "ffmpeg\ffprobe.exe" (
    echo Error: 'ffprobe.exe' not found in the 'ffmpeg' directory.
    echo Please ensure that 'ffmpeg.exe' and 'ffprobe.exe' are placed inside the 'ffmpeg' folder.
    pause
    exit /b 1
)

echo FFmpeg binaries found.

REM Step 5: Verify presence of tkdnd
echo Verifying presence of tkdnd directory...
if not exist "tkdnd" (
    echo Error: 'tkdnd' directory not found. Please ensure the 'tkdnd' directory is in the root of the project.
    pause
    exit /b 1
)

echo tkdnd directory found.

REM Step 6: Run PyInstaller with --onefile
echo Running PyInstaller...
pyinstaller --onefile --windowed --name "%APP_NAME%" audio_splitter_gui.py ^
--add-binary "ffmpeg\ffmpeg.exe;ffmpeg" ^
--add-binary "