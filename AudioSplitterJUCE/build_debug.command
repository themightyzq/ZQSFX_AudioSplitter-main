#!/bin/bash

# Build Debug - ZQ SFX Audio Splitter (JUCE version)
# This script builds a debug version for development

cd "$(dirname "$0")"

echo "🛠️  Building ZQ SFX Audio Splitter (Debug)..."
echo "=============================================="

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake is not installed. Please install CMake first."
    echo "   Download from: https://cmake.org/download/"
    exit 1
fi

# Only clean if user wants to
if [ "$1" = "clean" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf build
fi

# Configure with CMake (if needed)
if [ ! -d "build" ] || [ ! -f "build/CMakeCache.txt" ]; then
    echo "⚙️  Configuring build system..."
    if ! cmake -B build -DCMAKE_BUILD_TYPE=Debug; then
        echo "❌ CMake configuration failed!"
        exit 1
    fi
fi

echo "🔨 Building debug version..."
if ! cmake --build build --config Debug -j $(sysctl -n hw.ncpu); then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Debug build completed successfully!"
echo ""
echo "📦 Application built at:"
echo "   build/AudioSplitter_artefacts/Debug/ZQ SFX Audio Splitter.app"
echo ""
echo "🚀 To launch the debug version, run: ./Build_Launch.command"
echo ""
echo "💡 For faster builds, this script only reconfigures if needed."
echo "   Use './build_debug.command clean' to force a clean build."