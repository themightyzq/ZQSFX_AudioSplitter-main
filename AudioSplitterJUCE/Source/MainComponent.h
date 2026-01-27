#pragma once

#include <JuceHeader.h>
#include "Components/SingleFileSplitter.h"
#include "Components/BatchSplitter.h"
#include "Components/OptionsPanel.h"
#include "Components/ProgressPanel.h"
#include "Components/UCSNamingPanel.h"
#include "Components/CollapsiblePanel.h"
#include "Utils/ConfigManager.h"
#include "Utils/UCSManager.h"
#include "UI/ModernLookAndFeel.h"

//==============================================================================
/**
 * Main component class - mirrors Python notebook and main UI setup
 * 
 * Python equivalent: Lines 1202-1909 (notebook setup and main UI)
 * Responsible for:
 * - Tabbed interface (Single File Split / Batch Split)
 * - Options panel (sample rate, bit depth, naming)
 * - Progress bar
 * - Main UI layout and styling
 */
class MainComponent : public juce::Component
{
public:
    //==============================================================================
    /**
     * Constructor - mirrors Python main UI setup
     * @param configManager Configuration manager reference
     */
    explicit MainComponent(ConfigManager* configManager);
    
    ~MainComponent() override;

    //==============================================================================
    // Component interface - mirrors Python UI management
    
    /**
     * Paint background - mirrors Python dark theme setup (lines 1233-1310)
     */
    void paint(juce::Graphics& g) override;
    
    /**
     * Layout components - mirrors Python grid/pack layout system
     */
    void resized() override;

    //==============================================================================
    // Tab management - mirrors Python notebook behavior
    
    /**
     * Get current tab index - mirrors Python notebook.select()
     */
    int getCurrentTab() const;
    
    /**
     * Switch to tab - mirrors Python tab switching
     */
    void setCurrentTab(int tabIndex);
    
    /**
     * Tab changed callback - mirrors Python tab change events
     */
    void tabChanged();

    //==============================================================================
    // UI State management - mirrors Python update_button_states() (lines 136-168)
    
    /**
     * Update UI state based on current selections
     * Mirrors Python update_button_states() function exactly
     */
    void updateButtonStates();
    
    /**
     * Update file count display - mirrors Python update_file_count() (lines 474-480)
     */
    void updateFileCount();
    
    /**
     * Get options panel for accessing settings
     */
    OptionsPanel* getOptionsPanel() const { return optionsPanelContent; }

    /**
     * Get progress panel for updating progress
     */
    ProgressPanel* getProgressPanel() const { return progressPanel.get(); }

    /**
     * Get UCS naming panel for UCS settings
     */
    UCSNamingPanel* getUCSNamingPanel() const { return ucsNamingPanelContent; }

    /**
     * Get UCS manager for UCS operations
     */
    UCSManager* getUCSManager() const { return ucsManager.get(); }

private:
    //==============================================================================
    // Main UI components - mirrors Python UI structure
    
    // Tabbed interface - mirrors Python notebook (lines 1202-1210)
    juce::TabbedComponent tabbedComponent;
    
    // Tab components - mirrors Python tab setup
    std::unique_ptr<SingleFileSplitter> singleFileSplitter;
    std::unique_ptr<BatchSplitter> batchSplitter;

    // Options panel - mirrors Python options_frame (lines 1560-1701)
    // Wrapped in CollapsiblePanel for better UI/UX
    OptionsPanel* optionsPanelContent {nullptr};  // Raw pointer - owned by collapsibleOptionsPanel
    std::unique_ptr<CollapsiblePanel> collapsibleOptionsPanel;

    // UCS naming panel - professional sound effects naming
    // Wrapped in CollapsiblePanel for better UI/UX
    UCSNamingPanel* ucsNamingPanelContent {nullptr};  // Raw pointer - owned by collapsibleUCSPanel
    std::unique_ptr<CollapsiblePanel> collapsibleUCSPanel;

    // Progress panel - mirrors Python progress_bar setup (lines 1703-1716)
    std::unique_ptr<ProgressPanel> progressPanel;

    // Bottom button - mirrors Python split_button (lines 1722-1730)
    juce::TextButton splitButton;

    //==============================================================================
    // Configuration and state
    ConfigManager* configManager;
    std::unique_ptr<UCSManager> ucsManager;
    
    // Modern UI theme
    std::unique_ptr<ModernLookAndFeel> modernLookAndFeel;
    
    //==============================================================================
    // Colors - mirrors Python color constants (lines 1060-1062)
    static const juce::Colour backgroundColour;
    static const juce::Colour foregroundColour;
    static const juce::Colour accentColour;
    
    //==============================================================================
    // Helper methods
    void setupTabs();
    void setupOptionsPanel();
    void setupProgressPanel();
    void setupSplitButton();
    void setupColors();
    
    // Button callbacks - mirrors Python button command functions
    void splitButtonClicked();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};