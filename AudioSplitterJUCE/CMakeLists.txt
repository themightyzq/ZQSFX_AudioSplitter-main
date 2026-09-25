cmake_minimum_required(VERSION 3.22)

# Minimum macOS 11.0, set before project(). Without it the binary inherits the build
# machine's OS as its floor (a CI runner produced minos 26.0, unloadable on older Macs).
# 11.0 is the lowest a universal (arm64 + x86_64) binary can target.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(AudioSplitter VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Universal Binary (Intel + Apple Silicon). Off by default so local dev builds stay
# fast (host arch only); release builds and CI turn it on with -DZQ_MAC_UNIVERSAL=ON.
# Must be set before targets (incl. JUCE) are created. CLAUDE.md L.1 mandates this for release.
if(APPLE)
    option(ZQ_MAC_UNIVERSAL "Build a macOS Universal Binary (arm64 + x86_64)" OFF)
    if(ZQ_MAC_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
        message(STATUS "ZQ: building macOS Universal Binary (arm64 + x86_64)")
    endif()
endif()

# JUCE 8.0.8 via FetchContent, pinned by tag (house pattern; see ../CLAUDE.md section 6 and
# Project_SK4n/CMakeLists.txt). Never vendor JUCE in-tree or reference a sibling project's copy.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# The ZQ SFX house UI (tokens, LookAndFeel, knobs, fonts, the company mark) -- must come after
# JUCE is available. Pinned by tag: bump deliberately, then re-run splitter_ui_snapshot and
# compare docs/ui_before.png / docs/ui_after.png.
FetchContent_Declare(zqsfx_ui
    GIT_REPOSITORY https://github.com/themightyzq/zqsfx_ui.git
    GIT_TAG v0.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zqsfx_ui)

# Create the application
juce_add_gui_app(AudioSplitter
    PRODUCT_NAME "ZQ SFX Audio Splitter"
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"
    VERSION "1.0.0"
    DESCRIPTION "Professional multichannel audio splitting tool"
    BUNDLE_ID "com.zqsfx.audiosplitter"
    MICROPHONE_PERMISSION_ENABLED FALSE
)

# Core UI + engine sources: everything MainComponent needs to construct and render, with no
# application-shell dependency (no Main.cpp / AudioSplitterApplication.* / MainWindow.*). Shared
# between the real app target and the headless splitter_ui_snapshot tool (see below) so the
# snapshot renders the REAL component tree, not a recompiled stand-in.
set(AUDIOSPLITTER_CORE_SOURCES
    # Main window content
    Source/MainComponent.cpp
    Source/MainComponent.h

    # UI Components (Phase 1 - created)
    Source/Components/SingleFileSplitter.cpp
    Source/Components/SingleFileSplitter.h
    Source/Components/BatchSplitter.cpp
    Source/Components/BatchSplitter.h
    Source/Components/OptionsPanel.cpp
    Source/Components/OptionsPanel.h
    Source/Components/ProgressPanel.cpp
    Source/Components/ProgressPanel.h
    Source/Components/UCSNamingPanel.cpp
    Source/Components/UCSNamingPanel.h
    Source/Components/CollapsiblePanel.cpp
    Source/Components/CollapsiblePanel.h

    # Utilities (Phase 1 & 2 - created)
    Source/Utils/ConfigManager.cpp
    Source/Utils/ConfigManager.h
    Source/Utils/AudioAnalyzer.cpp
    Source/Utils/AudioAnalyzer.h
    Source/Utils/AudioFileProcessor.cpp
    Source/Utils/AudioFileProcessor.h
    Source/Utils/BroadcastChunkPreserver.cpp
    Source/Utils/BroadcastChunkPreserver.h
    Source/Utils/UCSManager.cpp
    Source/Utils/UCSManager.h
    Source/Utils/BatchJobManager.cpp
    Source/Utils/BatchJobManager.h
    Source/Utils/BatchProcessingJob.cpp
    Source/Utils/BatchProcessingJob.h

    # UI Theme (Modern UI implementation)
    Source/UI/ModernLookAndFeel.cpp
    Source/UI/ModernLookAndFeel.h
)

# Phase 1: Basic application shell - only include files that exist
target_sources(AudioSplitter PRIVATE
    # Main application
    Source/Main.cpp
    Source/AudioSplitterApplication.cpp
    Source/AudioSplitterApplication.h

    # Main window
    Source/MainWindow.cpp
    Source/MainWindow.h

    ${AUDIOSPLITTER_CORE_SOURCES}
)

# Embedded UCS taxonomy (~6 KB JSON): the UCS naming feature -- this product's entire reason to
# exist -- must keep working after packaging or relocation, when a Resources/ directory relative
# to the installed executable may no longer exist. Embedding it removes that filesystem
# dependency entirely. Shared between the real app and the splitter_ui_snapshot tool since both
# compile AUDIOSPLITTER_CORE_SOURCES (which includes UCSManager.cpp).
juce_add_binary_data(AudioSplitterBinaryData
    HEADER_NAME "AudioSplitterBinaryData.h"
    NAMESPACE   "AudioSplitterBinaryData"
    SOURCES
        Resources/ucs_taxonomy.json
)
set_target_properties(AudioSplitterBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Tests (opt-in: ON by default, build the test target explicitly to run it)
option(ZQ_BUILD_TESTS "Build ZQ SFX test targets" ON)
if(ZQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# JUCE modules - minimal set for Phase 1
target_link_libraries(AudioSplitter PRIVATE
    AudioSplitterBinaryData         # Embedded UCS taxonomy JSON
    zqsfx::ui                       # ZQ SFX house UI (tokens, LookAndFeel, LogoMark)
    juce::juce_core                 # Core functionality
    juce::juce_data_structures      # JSON config
    juce::juce_events               # Event handling
    juce::juce_graphics             # UI graphics
    juce::juce_gui_basics           # Basic GUI
    juce::juce_gui_extra            # File choosers, tooltips
    juce::juce_audio_basics         # Audio data structures
    juce::juce_audio_formats        # Audio file I/O
    juce::juce_audio_utils          # Audio utilities
)

# Compiler definitions - match Python behavior
target_compile_definitions(AudioSplitter PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AudioSplitter,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AudioSplitter,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# Generate legacy header
juce_generate_juce_header(AudioSplitter)

# --- Headless UI render: the look-and-feel regression gate for the ZQ SFX house-UI migration ---
# This is a standalone app (no AudioProcessor/editor), so the tool constructs MainComponent
# directly, compiling the same core sources the real app uses (DePump's ui_snapshot pattern for
# apps) rather than a recompiled stand-in.
juce_add_console_app(splitter_ui_snapshot PRODUCT_NAME "splitter_ui_snapshot")

target_sources(splitter_ui_snapshot PRIVATE
    tools/ui_snapshot/main.cpp
    ${AUDIOSPLITTER_CORE_SOURCES}
)

target_include_directories(splitter_ui_snapshot PRIVATE Source)

target_compile_definitions(splitter_ui_snapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(splitter_ui_snapshot PRIVATE
    AudioSplitterBinaryData         # Embedded UCS taxonomy JSON (UCSManager.cpp needs it too)
    zqsfx::ui
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_utils
)

juce_generate_juce_header(splitter_ui_snapshot)

# Platform-specific settings
if(APPLE)
    set_target_properties(AudioSplitter PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zqsfx.audiosplitter"
        MACOSX_BUNDLE_BUNDLE_NAME "ZQ SFX Audio Splitter"
    )
elseif(WIN32)
    # Windows-specific settings
    target_compile_definitions(AudioSplitter PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()