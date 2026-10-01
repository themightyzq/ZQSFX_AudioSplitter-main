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
    
    // Window dimensions, owned by MainComponent so the snapshot tool and the layout test use the
    // same numbers. Sized for a 13-inch laptop; a window smaller than the content scrolls.
    static constexpr int defaultWidth = MainComponent::kDefaultWidth;
    static constexpr int defaultHeight = MainComponent::kDefaultHeight;
    static constexpr int minWidth = MainComponent::kMinWidth;
    static constexpr int minHeight = MainComponent::kMinHeight;
    
    //==============================================================================
    // Helper methods
    void setupWindow();
    void centerWindow();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};