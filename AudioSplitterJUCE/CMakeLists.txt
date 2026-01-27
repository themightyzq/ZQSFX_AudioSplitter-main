cmake_minimum_required(VERSION 3.22)

project(AudioSplitter VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory - JUCE should be in ./JUCE/
add_subdirectory(JUCE)

# Create the application
juce_add_gui_app(AudioSplitter
    PRODUCT_NAME "ZQ SFX Audio Splitter"
    COMPANY_NAME "ZQ Audio"
    VERSION "1.0.0"
    DESCRIPTION "Professional multichannel audio splitting tool"
    BUNDLE_ID "com.zqaudio.audiosplitter"
    MICROPHONE_PERMISSION_ENABLED FALSE
)

# Phase 1: Basic application shell - only include files that exist
target_sources(AudioSplitter PRIVATE
    # Main application
    Source/Main.cpp
    Source/AudioSplitterApplication.cpp
    Source/AudioSplitterApplication.h
    
    # Main window and components
    Source/MainWindow.cpp
    Source/MainWindow.h
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

# JUCE modules - minimal set for Phase 1
target_link_libraries(AudioSplitter PRIVATE
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

# Platform-specific settings
if(APPLE)
    set_target_properties(AudioSplitter PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zqaudio.audiosplitter"
        MACOSX_BUNDLE_BUNDLE_NAME "ZQ SFX Audio Splitter"
    )
elseif(WIN32)
    # Windows-specific settings
    target_compile_definitions(AudioSplitter PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()