#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

APP_NAME="ZQ SFX Audio Splitter"
DIST_DIR="dist"
BUILD_DIR="build"

echo "Starting build process..."

# Step 1: Terminate any running instances of the application
echo "Terminating any running instances of '$APP_NAME'..."
pkill "$APP_NAME" || echo "No running instances found."

# Step 2: Clean previous build artifacts
echo "Cleaning previous build directories..."
rm -rf "$DIST_DIR" "$BUILD_DIR" || echo "No previous build directories found."

# Step 3: Remove any existing spec files
SPEC_FILE="$(ls *.spec 2>/dev/null | head -n 1)"
if [ -n "$SPEC_FILE" ]; then
    echo "Removing existing spec file: $SPEC_FILE"
    rm "$SPEC_FILE"
else
    echo "No spec file found."
fi

# Step 4: Verify FFmpeg Binaries
echo "Verifying presence of FFmpeg binaries..."
if [ ! -f "ffmpeg/ffmpeg" ] || [ ! -f "ffmpeg/ffprobe" ]; then
    echo "Error: 'ffmpeg' and/or 'ffprobe' binaries not found in the 'ffmpeg' directory."
    echo "Please ensure that both 'ffmpeg' and 'ffprobe' are placed inside the 'ffmpeg' folder."
    exit 1
fi
echo "FFmpeg binaries found."

# Step 5: Run PyInstaller with --onefile
echo "Running PyInstaller..."
pyinstaller --onefile --windowed --name "$APP_NAME" audio_splitter_gui.py \
    --add-binary "ffmpeg/ffmpeg:ffmpeg" \
    --add-binary "ffmpeg/ffprobe:ffmpeg" \
    --hidden-import=tkinter \
    --hidden-import=pydub.utils \
    --hidden-import=pydub \
    --hidden-import=numpy \
    --log-level=DEBUG

echo "Build process completed successfully."
