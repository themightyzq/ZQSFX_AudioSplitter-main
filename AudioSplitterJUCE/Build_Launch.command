#!/bin/bash

# Build Launch - ZQ SFX Audio Splitter (JUCE version)
# This script launches the latest built version of the application

cd "$(dirname "$0")"

echo "🚀 Launching ZQ SFX Audio Splitter..."
echo "====================================="

# Check if the application exists (try both Release and Debug)
RELEASE_APP="build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.app"
DEBUG_APP="build/AudioSplitter_artefacts/Debug/ZQ SFX Audio Splitter.app"

if [ -d "$RELEASE_APP" ]; then
    echo "🎯 Launching Release version..."
    open "$RELEASE_APP"
elif [ -d "$DEBUG_APP" ]; then
    echo "🛠️  Launching Debug version..."
    open "$DEBUG_APP"
else
    echo "❌ Application not found!"
    echo "   Please run './Build_Create.command' first to build the application."
    echo ""
    echo "   Expected locations:"
    echo "   - $RELEASE_APP"
    echo "   - $DEBUG_APP"
    exit 1
fi

echo "✅ Application launched successfully!"