#include "AudioSplitterApplication.h"
#include <iostream>

//==============================================================================
void AudioSplitterApplication::initialise(const juce::String& commandLine)
try
{
    // Initialize logging system - mirrors Python setup_logging() (lines 117-134)
    juce::Logger::setCurrentLogger(juce::FileLogger::createDefaultAppLogger(
        "ZQSFXAudioSplitter", "app.log", "ZQ SFX Audio Splitter Log"));
    
    juce::Logger::writeToLog("ZQ SFX Audio Splitter application started.");
    
    // Initialize configuration manager - mirrors Python load_config() (lines 614-627)
    configManager = std::make_unique<ConfigManager>();
    configManager->loadConfig();
    
    // Get last directories from config - mirrors Python global variables (lines 200-202)
    lastInputDir = configManager->getLastInputDir();
    lastOutputDir = configManager->getLastOutputDir();
    
    juce::Logger::writeToLog("Configuration loaded successfully.");
    
    // Create main window - mirrors Python root = TkinterDnD.Tk() setup (lines 1193-1197)
    mainWindow = std::make_unique<MainWindow>(
        getApplicationName(),
        configManager.get()
    );
    
    juce::Logger::writeToLog("Main window created successfully.");
}
catch (const std::exception& e)
{
    // Mirror Python exception handling pattern used throughout
    std::cerr << "Error during application initialization: " << e.what() << std::endl;
    juce::Logger::writeToLog("Fatal error during initialization: " + juce::String(e.what()));
    quit();
}

//==============================================================================
void AudioSplitterApplication::shutdown()
{
    // Save configuration - mirrors Python on_closing() function (lines 639-642)
    if (configManager != nullptr)
    {
        configManager->saveConfig();
        juce::Logger::writeToLog("Configuration saved. Exiting application.");
    }
    
    // Clean up main window
    mainWindow = nullptr;
    configManager = nullptr;
    
    // Clean up logging
    juce::Logger::setCurrentLogger(nullptr);
}

//==============================================================================
void AudioSplitterApplication::systemRequestedQuit()
{
    // Handle system quit request - mirrors Python WM_DELETE_WINDOW protocol (line 1228)
    quit();
}

//==============================================================================
void AudioSplitterApplication::anotherInstanceStarted(const juce::String& commandLine)
{
    // Handle multiple instances - mirrors Python moreThanOneInstanceAllowed behavior
    // Currently allows multiple instances like the Python version
    juce::Logger::writeToLog("Another instance started with command: " + commandLine);
}

//==============================================================================
void AudioSplitterApplication::unhandledException(const std::exception* e, 
                                                  const juce::String& sourceFilename, 
                                                  int lineNumber)
{
    // Global exception handler - mirrors Python try/except patterns throughout code
    juce::String errorMsg = "Unhandled exception";
    if (e != nullptr)
    {
        errorMsg += ": " + juce::String(e->what());
    }
    errorMsg += " at " + sourceFilename + ":" + juce::String(lineNumber);
    
    juce::Logger::writeToLog("FATAL ERROR - " + errorMsg);
    std::cerr << errorMsg << std::endl;
    
    // Show error dialog to user - mirrors Python messagebox.showerror() calls
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Error",
        "An unexpected error occurred:\n" + errorMsg,
        "OK"
    );
}