@echo off

REM Build Launch - ZQ SFX Audio Splitter (JUCE version)
REM This script launches the latest built version of the application

cd /d "%~dp0"

echo 🚀 Launching ZQ SFX Audio Splitter...
echo =====================================

REM Check if the application exists (try both Release and Debug)
set "RELEASE_APP=build\AudioSplitter_artefacts\Release\ZQ SFX Audio Splitter.exe"
set "DEBUG_APP=build\AudioSplitter_artefacts\Debug\ZQ SFX Audio Splitter.exe"

if exist "%RELEASE_APP%" (
    echo 🎯 Launching Release version...
    start "" "%RELEASE_APP%"
    echo ✅ Application launched successfully!
) else if exist "%DEBUG_APP%" (
    echo 🛠️  Launching Debug version...
    start "" "%DEBUG_APP%"
    echo ✅ Application launched successfully!
) else (
    echo ❌ Application not found!
    echo    Please run 'Build_Create.bat' first to build the application.
    echo.
    echo    Expected locations:
    echo    - %RELEASE_APP%
    echo    - %DEBUG_APP%
    pause
    exit /b 1
)