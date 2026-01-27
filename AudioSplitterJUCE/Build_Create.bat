@echo off

REM Build Create - ZQ SFX Audio Splitter (JUCE version)
REM This script configures and builds the latest version of the audio splitter

cd /d "%~dp0"

echo 🔨 Building ZQ SFX Audio Splitter (JUCE version)...
echo ==================================================

REM Check if CMake is available
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ❌ CMake is not installed. Please install CMake first.
    echo    Download from: https://cmake.org/download/
    pause
    exit /b 1
)

REM Clean previous build
if exist "build" (
    echo 🧹 Cleaning previous build...
    rmdir /s /q build
)

REM Configure with CMake
echo ⚙️  Configuring build system...
cmake -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ❌ CMake configuration failed!
    pause
    exit /b 1
)

REM Build the application
echo 🔨 Building application...
cmake --build build --config Release
if errorlevel 1 (
    echo ❌ Build failed!
    pause
    exit /b 1
)

echo ✅ Build completed successfully!
echo.
echo 📦 Application built at:
echo    build\AudioSplitter_artefacts\Release\ZQ SFX Audio Splitter.exe
echo.
echo 🚀 To launch the application, run: Build_Launch.bat
echo.

pause