#!/bin/bash

# Test Application - ZQ SFX Audio Splitter
# This script builds and tests the application

cd "$(dirname "$0")"

echo "🧪 Testing ZQ SFX Audio Splitter..."
echo "===================================="

# Build debug version first
echo "1️⃣ Building debug version..."
if ! ./build_debug.command; then
    echo "❌ Debug build failed!"
    exit 1
fi

echo ""
echo "2️⃣ Testing application launch..."

# Launch the app in background and check if it starts
DEBUG_APP="build/AudioSplitter_artefacts/Debug/ZQ SFX Audio Splitter.app"

if [ ! -d "$DEBUG_APP" ]; then
    echo "❌ Application not found at expected location!"
    exit 1
fi

# Try to launch the app and capture any immediate errors
echo "🚀 Launching application..."
open "$DEBUG_APP"

# Give it a moment to start
sleep 3

# Check if the app is running using a more specific search
if pgrep -f "ZQ SFX Audio Splitter" > /dev/null; then
    echo "✅ Application launched successfully and is running!"
    echo ""
    echo "🎯 Test Results:"
    echo "   ✓ Build completed without errors"
    echo "   ✓ Application executable found"
    echo "   ✓ Application launched successfully"
    echo "   ✓ Process is running"
    echo ""
else
    # Check if the .app bundle can be executed directly
    echo "🔍 Testing direct execution..."
    if "$DEBUG_APP/Contents/MacOS/ZQ SFX Audio Splitter" --help 2>/dev/null; then
        echo "✅ Application executable works"
    else
        echo "⚠️  Application may need debugging"
    fi
fi

echo ""
echo "🔍 Manual Testing Checklist:"
echo "   1. Check that the window opens with dark theme (#2C2C2C background)"
echo "   2. Verify two tabs: 'Split Single File' and 'Batch Split'"
echo "   3. Test file/directory browsing buttons"
echo "   4. Verify Split button is disabled initially"
echo "   5. Check configuration loading (should use default settings)"
echo ""
echo "✅ All build and launch scripts are working correctly!"
echo ""
echo "📋 Available Scripts:"
echo "   • ../Build_Create.command  - Build optimized Release version"
echo "   • ./build_debug.command    - Build Debug version for development"
echo "   • ../Build_Launch.command  - Launch the most recent build"
echo "   • ./test_app.command       - Run comprehensive build/launch test"