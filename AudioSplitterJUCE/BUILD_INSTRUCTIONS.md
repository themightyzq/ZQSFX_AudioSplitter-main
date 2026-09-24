# Build Instructions - ZQ SFX Audio Splitter (JUCE)

This document describes how to build and launch the JUCE version of the ZQ SFX Audio Splitter.

## Prerequisites

- **CMake 3.22+**: Download from [cmake.org](https://cmake.org/download/)
- **Xcode** (macOS) or **Visual Studio** (Windows) for compilation
- **Git** (included with Xcode or downloadable separately)

### Dependencies

- **JUCE 8.0.8** is fetched automatically by CMake (`FetchContent`, pinned by tag in
  `AudioSplitterJUCE/CMakeLists.txt`) — the first configure needs network access to clone it;
  after that it is cached in `AudioSplitterJUCE/build/_deps/` and reused.
- To update JUCE, bump the `GIT_TAG` in `AudioSplitterJUCE/CMakeLists.txt` and do a clean
  rebuild. Pin changes should be deliberate and tested — the metadata-preservation tests in
  `AudioSplitterJUCE/Tests/` guard against WAV/iXML behavior regressions across JUCE versions.

### Build options

| CMake option | Default | Purpose |
|--------------|---------|---------|
| `CMAKE_BUILD_TYPE` | `Release` (via scripts) | `Release` or `Debug` |
| `ZQ_MAC_UNIVERSAL` | `OFF` | macOS only: build a Universal Binary (arm64 + x86_64). Required for release/distribution; CI builds with this `ON`. Local dev leaves it off for faster builds. |
| `ZQ_BUILD_TESTS` | `ON` | Build the test targets (run them with `ctest`). |

**Universal Binary (macOS release):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DZQ_MAC_UNIVERSAL=ON
cmake --build build --config Release --parallel
```

**Run the tests:**
```bash
ctest --test-dir build --build-config Release --output-on-failure
```

## Quick Start

### macOS (.command files)

```bash
# Build optimized release version
./Build_Create.command

# Launch the application
./Build_Launch.command
```

### Windows (.bat files)

```cmd
# Build optimized release version
Build_Create.bat

# Launch the application
Build_Launch.bat
```

## Available Scripts

### Build Scripts

| Script | Platform | Purpose |
|--------|----------|---------|
| `Build_Create.command` | macOS | Build optimized Release version |
| `Build_Create.bat` | Windows | Build optimized Release version |
| `build_debug.command` | macOS | Build Debug version for development |

### Launch Scripts

| Script | Platform | Purpose |
|--------|----------|---------|
| `Build_Launch.command` | macOS | Launch most recent build (Release or Debug) |
| `Build_Launch.bat` | Windows | Launch most recent build (Release or Debug) |

### Test Scripts

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_app.command` | macOS | Comprehensive build and launch test |

## Development Workflow

### For Release Builds
1. Run `./Build_Create.command` (macOS) or `Build_Create.bat` (Windows)
2. Run `./Build_Launch.command` (macOS) or `Build_Launch.bat` (Windows)

### For Development (Debug Builds)
1. Run `./build_debug.command` (macOS only)
2. Run `./Build_Launch.command` to launch debug version
3. For faster iteration, just run `./build_debug.command` again (skips configuration)
4. Use `./build_debug.command clean` to force a complete rebuild

## Manual Build (Advanced)

If you prefer to build manually:

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j 4

# Launch (macOS)
open "build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.app"

# Launch (Windows)
"build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.exe"
```

## Build Outputs

### macOS
- **Release**: `build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.app`
- **Debug**: `build/AudioSplitter_artefacts/Debug/ZQ SFX Audio Splitter.app`

### Windows
- **Release**: `build/AudioSplitter_artefacts/Release/ZQ SFX Audio Splitter.exe`
- **Debug**: `build/AudioSplitter_artefacts/Debug/ZQ SFX Audio Splitter.exe`

## Troubleshooting

### CMake Not Found
- **macOS**: Install Xcode Command Line Tools: `xcode-select --install`
- **Windows**: Download and install CMake from [cmake.org](https://cmake.org/download/)

### Build Fails
1. Clean build directory: `rm -rf build` (macOS) or `rmdir /s build` (Windows)
2. Re-run the build script
3. Check that all prerequisites are installed

### Application Won't Launch
1. Check Console.app (macOS) or Event Viewer (Windows) for error messages
2. Try building and launching the Debug version for more detailed error information
3. Verify all JUCE dependencies are properly linked

## Testing Checklist

After building and launching, verify:

1. ✅ Application window opens with dark theme (#2C2C2C background)
2. ✅ Two tabs visible: "Split Single File" and "Batch Split"  
3. ✅ File/directory browsing buttons work
4. ✅ Split button is disabled initially (correct state)
5. ✅ Configuration loads default settings
6. ✅ Drag-and-drop areas are responsive

## Development Notes

- Debug builds include additional logging and assertions
- Release builds are optimized for performance and distribution
- The build system automatically handles JUCE module compilation
- All scripts include error checking and user feedback

