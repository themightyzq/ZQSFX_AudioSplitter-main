#pragma once

#include <JuceHeader.h>
#include "MainComponent.h"
#include "Utils/ConfigManager.h"

//==============================================================================
/**
 * Main window class - mirrors Python Tkinter root window setup
 * 
 * Python equivalent: Lines 1193-1228 (Tkinter window setup)
 * Responsible for:
 * - Main application window
 * - Window sizing and positioning
 * - Window close handling
 * - Housing the main component
 */
class MainWindow : public juce::DocumentWindow
{
public:
    //==============================================================================
    /**
     * Constructor - mirrors Python root window setup
     * @param name Application name (mirrors root.title())
     * @param configManager Configuration manager reference
     */
    MainWindow(const juce::String& name, ConfigManager* configManager);
    
    ~MainWindow() override;

    //==============================================================================
    // DocumentWindow interface - mirrors Python window behavior
    
    /**
     * Close button pressed - mirrors Python WM_DELETE_WINDOW protocol
     * Equivalent to on_closing() function (lines 639-642)
     */
    void closeButtonPressed() override;
    
    /**
     * Window moved - save position like Python remembers directories
     */
    void moved() override;
    
    /**
     * Window resized - mirrors Python window geometry management
     */
    void resized() override;

private:
    //==============================================================================
    // Main component container - mirrors Python notebook setup
    std::unique_ptr<MainComponent> mainComponent;
    
    // Configuration manager reference
    ConfigManager* configManager;
    
    // Window state - much larger default size for proper UI display
    static constexpr int defaultWidth = 1200;   // Much larger for proper content visibility
    static constexpr int defaultHeight = 900;   // Much larger for proper content visibility  
    static constexpr int minWidth = 1000;       // Larger minimum to ensure all content is visible
    static constexpr int minHeight = 750;       // Larger minimum to prevent any content cutoff
    
    //==============================================================================
    // Helper methods
    void setupWindow();
    void centerWindow();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};