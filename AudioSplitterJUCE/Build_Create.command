#!/bin/bash

# Build Create - ZQ SFX Audio Splitter (JUCE version)
# This script configures and builds the latest version of the audio splitter

cd "$(dirname "$0")"

echo "🔨 Building ZQ SFX Audio Splitter (JUCE version)..."
echo "=================================================="

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake is not installed. Please install CMake first."
    echo "   Download from: https://cmake.org/download/"
    exit 1
fi

# Clean previous build
if [ -d "build" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf build
fi

# Configure with CMake
echo "⚙️  Configuring build system..."
if ! cmake -B build -DCMAKE_BUILD_TYPE=Release; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

echo "🔨 Building application..."
if ! cmake --build build --config Release -j $(sysctl -n hw.ncpu); then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build completed successfully!"
echo ""
echo "📦 Application built at:"
echo "   build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.app"
echo ""
echo "🚀 To launch the application, run: ./Build_Launch.command"
echo ""

# Make launch script executable
chmod +x Build_Launch.command

echo "Press any key to continue..."
read -n 1 -s