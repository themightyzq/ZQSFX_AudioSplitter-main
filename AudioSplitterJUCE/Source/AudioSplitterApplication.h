#pragma once

#include <JuceHeader.h>
#include "MainWindow.h"
#include "Utils/ConfigManager.h"

//==============================================================================
/**
 * Main application class - mirrors Python main() function behavior
 * 
 * Python equivalent: main() function and application setup
 * Responsible for:
 * - Application lifecycle management
 * - Configuration loading/saving
 * - Main window creation
 * - Global error handling
 */
class AudioSplitterApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    AudioSplitterApplication() = default;
    ~AudioSplitterApplication() override = default;

    //==============================================================================
    // JUCEApplication interface - mirrors Python application lifecycle

    const juce::String getApplicationName() override 
    { 
        return "ZQ SFX Audio Splitter"; 
    }
    
    const juce::String getApplicationVersion() override 
    { 
        return "1.0.0"; 
    }
    
    bool moreThanOneInstanceAllowed() override 
    { 
        return true; 
    }

    //==============================================================================
    // Application lifecycle - mirrors Python setup and teardown
    
    /**
     * Initialize application - mirrors Python main() setup section
     * Equivalent to lines 1183-1228 in Python (load_config, setup_logging, etc.)
     */
    void initialise(const juce::String& commandLine) override;

    /**
     * Shutdown application - mirrors Python on_closing() function  
     * Equivalent to lines 639-642 in Python (save_config, cleanup)
     */
    void shutdown() override;

    //==============================================================================
    // System events - mirrors Python system integration
    
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;

    //==============================================================================
    // Global error handling - mirrors Python exception handling
    void unhandledException(const std::exception* e, 
                           const juce::String& sourceFilename, 
                           int lineNumber) override;

private:
    //==============================================================================
    // Core application components
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<ConfigManager> configManager;
    
    // Global state (mirrors Python global variables)
    juce::String lastInputDir;
    juce::String lastOutputDir;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSplitterApplication)
};