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
    // Configuration manager reference
    ConfigManager* configManager;
    
    // Window dimensions. 1110 is what MainComponent::resized() needs to show both collapsible
    // panels at their ideal height with the 200 px tab floor; below that it scales the panels
    // down and the UCS preview row is the first thing to clip.
    static constexpr int defaultWidth = 1200;
    static constexpr int defaultHeight = 1110;
    static constexpr int minWidth = 1000;
    static constexpr int minHeight = 850;
    
    //==============================================================================
    // Helper methods
    void setupWindow();
    void centerWindow();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};