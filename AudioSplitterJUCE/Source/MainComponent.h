#pragma once

#include <JuceHeader.h>
#include <zqsfx_ui/zqsfx_ui.h>
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
    // Installs the ZQ SFX house LookAndFeel (zqsfx_ui, via ModernLookAndFeel) as the JUCE
    // default for its lifetime. Must outlive every component, so the application owns one ahead
    // of its window (AudioSplitterApplication.h/.cpp), and splitter_ui_snapshot uses the same
    // pattern (DePump's MainComponent::ScopedHouseLookAndFeel is the worked example for apps).
    struct ScopedHouseLookAndFeel
    {
        ScopedHouseLookAndFeel() { juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel); }
        ~ScopedHouseLookAndFeel() { juce::LookAndFeel::setDefaultLookAndFeel(nullptr); }
        ModernLookAndFeel lookAndFeel;
    };

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

    /**
     * Handle keyboard shortcuts (Cmd+Return to split)
     */
    bool keyPressed(const juce::KeyPress& key) override;

    /** Keeps the focused control in view: keyboard Tab can reach controls that are scrolled out. */
    void focusOfChildComponentChanged(FocusChangeType cause) override;

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

    /** Whether the Split button can be pressed right now (for the split-state test). */
    bool isSplitEnabled() const { return splitButton.isEnabled(); }

    /** The two tabs, for the layout test and snapshot tool (load a file, switch tabs). */
    SingleFileSplitter* getSingleFileSplitter() const { return singleFileSplitter.get(); }
    BatchSplitter* getBatchSplitter() const { return batchSplitter.get(); }

    /** Window size limits sized for a 13-inch laptop (a 1280x800 screen leaves about 1280x775
        below the menu bar, less the title bar). Smaller than the content needs? It scrolls. */
    static constexpr int kDefaultWidth = 1180;
    static constexpr int kDefaultHeight = 740;
    static constexpr int kMinWidth = 960;
    static constexpr int kMinHeight = 640;

private:
    //==============================================================================
    // Main UI components - mirrors Python UI structure
    
    // Everything between the header and the bottom bar (tabs, Options, UCS Naming) lives in one
    // scrolling column, so a short window scrolls instead of clipping. Declared before the
    // components it holds so it outlives them. The progress bar and Split stay fixed below.
    struct Body : public juce::Component {};
    Body body;
    juce::Viewport bodyViewport;

    // Tabbed interface - mirrors Python notebook (lines 1202-1210). TabbedComponent only tells
    // a subclass when the tab changes, and Split's enabled state depends on the active tab.
    struct SplitterTabs : public juce::TabbedComponent
    {
        using juce::TabbedComponent::TabbedComponent;
        void currentTabChanged(int, const juce::String&) override
        {
            if (onTabChanged)
                onTabChanged();
        }
        std::function<void()> onTabChanged;
    };
    SplitterTabs tabbedComponent;
    
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

    // House chrome: the company mark in the header, far right, doubles as the About trigger.
    zqsfx::ui::LogoMark logo {"Audio Splitter"};

    //==============================================================================
    // Helper methods
    void setupTabs();
    void setupOptionsPanel();
    void setupProgressPanel();
    void setupSplitButton();

    // Scrolling-body layout. measureBody() is the height the column needs at a width; layoutBody()
    // sizes the column (scrolling when it is taller than the viewport) and places its parts.
    int getTabsPreferredHeight(int width) const;
    int getPanelsPreferredHeight(int width) const;
    void layoutBody();
    void setupColors();
    void showAboutBox();

    // Button callbacks - mirrors Python button command functions
    void splitButtonClicked();

    /** Progress panel's Cancel button: stops whichever split is running. */
    void cancelRunningOperation();

    bool isSplitRunning() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};