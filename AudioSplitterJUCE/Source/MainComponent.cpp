#include "MainComponent.h"

//==============================================================================
// Color constants - mirrors Python color definitions (lines 1060-1062)
const juce::Colour MainComponent::backgroundColour = juce::Colour(0xff2c2c2c); // "#2C2C2C"
const juce::Colour MainComponent::foregroundColour = juce::Colour(0xffffffff); // "#FFFFFF"
const juce::Colour MainComponent::accentColour = juce::Colour(0xff3c3c3c);     // "#3C3C3C"

//==============================================================================
MainComponent::MainComponent(ConfigManager* config)
    : configManager(config),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop), // mirrors Python notebook orientation
      splitButton("Split") // mirrors Python split_button text (line 1724)
{
    // Create and apply modern look and feel
    modernLookAndFeel = std::make_unique<ModernLookAndFeel>();
    setLookAndFeel(modernLookAndFeel.get());
    
    // Initialize UCS system first
    ucsManager = std::make_unique<UCSManager>();
    if (!ucsManager->loadTaxonomyFromBinaryData())
    {
        juce::Logger::writeToLog("Warning: Failed to load UCS taxonomy");
    }

    // Setup UI components in same order as Python - BEFORE setting size
    setupTabs();        // mirrors Python notebook setup (lines 1202-1210)
    setupOptionsPanel(); // mirrors Python options_frame setup (lines 1560-1701)
    setupProgressPanel(); // mirrors Python progress_bar setup (lines 1703-1716)
    setupSplitButton();  // mirrors Python split_button setup (lines 1722-1738)
    setupColors();       // mirrors Python styling setup (lines 1233-1310)

    // Setup UCS naming panel wrapped in collapsible panel
    ucsNamingPanelContent = new UCSNamingPanel(ucsManager.get(), configManager);
    ucsNamingPanelContent->onSettingsChanged = [this]() {
        updateButtonStates();
    };

    collapsibleUCSPanel = std::make_unique<CollapsiblePanel>(
        "UCS Naming",
        ucsNamingPanelContent,
        configManager,
        "ucs_panel_collapsed",
        false  // start expanded
    );

    collapsibleUCSPanel->onCollapseChanged = [this](bool /*isCollapsed*/) {
        resized();  // Re-layout when panel collapses/expands
    };

    // Add components to layout
    addAndMakeVisible(tabbedComponent);
    addAndMakeVisible(*collapsibleOptionsPanel);
    addAndMakeVisible(*collapsibleUCSPanel);
    addAndMakeVisible(*progressPanel);
    addAndMakeVisible(splitButton);
    
    // Don't set size here - let the parent window control the size
    // The MainWindow will set the appropriate size
    
    // Initial UI state update - mirrors Python initial state
    updateButtonStates();
    
    juce::Logger::writeToLog("Main component initialized successfully");
}

//==============================================================================
MainComponent::~MainComponent()
{
    // Reset look and feel before destruction
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Use modern background color
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Early return if components aren't initialized yet
    if (!collapsibleOptionsPanel || !progressPanel || !collapsibleUCSPanel)
        return;

    // Layout using modern spacing system - generous spacing for proper UI display
    const int margin = ModernLookAndFeel::Spacing::md;           // 16px modern spacing
    const int minOptionsHeight = 180;                            // Reduced since UCS panel is separate
    const int minUCSHeight = 200;                                // UCS naming panel height
    const int minProgressHeight = 100;                           // Larger for better visibility
    const int minButtonHeight = 60;                              // Larger for better touch targets

    // Calculate responsive heights based on available space
    int totalHeight = bounds.getHeight() - (margin * 2);
    int optionsPanelHeight = juce::jmax(minOptionsHeight, totalHeight / 5);     // 1/5 of height
    int ucsPanelHeight = juce::jmax(minUCSHeight, totalHeight / 5);             // 1/5 of height
    int progressPanelHeight = juce::jmax(minProgressHeight, totalHeight / 10);  // 1/10 of height
    // PRIMARY ACTION BUTTON: Make it LARGE and OBVIOUS (increased from 1/15 to 1/12 of height)
    int buttonHeight = juce::jmax(50, totalHeight / 12);  // Minimum 50px, larger proportion

    bounds.reduce(margin, margin);

    // Split button at bottom - prominent primary action
    splitButton.setBounds(bounds.removeFromBottom(buttonHeight).reduced(ModernLookAndFeel::Spacing::sm));
    bounds.removeFromBottom(ModernLookAndFeel::Spacing::sm); // spacing

    // Progress panel above button - modern spacing
    progressPanel->setBounds(bounds.removeFromBottom(progressPanelHeight));
    bounds.removeFromBottom(ModernLookAndFeel::Spacing::sm); // spacing

    // COLLAPSIBLE PANELS: Use ideal height (accounts for collapsed/expanded state)
    // UCS naming panel above progress (dynamically sized)
    ucsPanelHeight = collapsibleUCSPanel->getIdealHeight();
    collapsibleUCSPanel->setBounds(bounds.removeFromBottom(ucsPanelHeight));
    bounds.removeFromBottom(ModernLookAndFeel::Spacing::sm); // spacing

    // Options panel above UCS naming (dynamically sized)
    optionsPanelHeight = collapsibleOptionsPanel->getIdealHeight();
    collapsibleOptionsPanel->setBounds(bounds.removeFromBottom(optionsPanelHeight));
    bounds.removeFromBottom(ModernLookAndFeel::Spacing::sm); // spacing

    // Tabs take remaining space - mirrors Python notebook.pack(fill="both", expand=True)
    tabbedComponent.setBounds(bounds);
}

//==============================================================================
void MainComponent::setupTabs()
{
    // Create tab components - mirrors Python tab creation (lines 1206-1210)
    singleFileSplitter = std::make_unique<SingleFileSplitter>(configManager);
    batchSplitter = std::make_unique<BatchSplitter>(configManager);
    
    // Connect file analysis callbacks - mirrors Python update_button_states() calls (lines 667-668)
    singleFileSplitter->onFileAnalyzed = [this]() {
        updateButtonStates();
    };
    
    batchSplitter->onDirectoryAnalyzed = [this]() {
        updateButtonStates();
    };
    
    // Add tabs with same names and colors as Python
    tabbedComponent.addTab(
        "Split Single File",        // mirrors Python text (line 1209)
        backgroundColour,           // tab background
        singleFileSplitter.get(),   // tab content
        false                       // don't delete on removal
    );
    
    tabbedComponent.addTab(
        "Batch Split",              // mirrors Python text (line 1210)
        backgroundColour,           // tab background  
        batchSplitter.get(),        // tab content
        false                       // don't delete on removal
    );
    
    // Set tab change callback - mirrors Python tab selection handling
    tabbedComponent.setTabBarDepth(30); // reasonable tab height
    
    // Set initial tab (mirrors Python default selection)
    tabbedComponent.setCurrentTabIndex(0);
}

//==============================================================================
void MainComponent::setupOptionsPanel()
{
    // Create options panel - mirrors Python options_frame (lines 1560-1701)
    optionsPanelContent = new OptionsPanel(configManager);

    // Connect change callbacks to update button states
    // This mirrors Python variable tracing (lines 1901-1903)
    optionsPanelContent->onSettingsChanged = [this]() {
        updateButtonStates();
    };

    // Wrap in collapsible panel for better UI/UX
    collapsibleOptionsPanel = std::make_unique<CollapsiblePanel>(
        "Options",
        optionsPanelContent,
        configManager,
        "options_panel_collapsed",
        false  // start expanded
    );

    collapsibleOptionsPanel->onCollapseChanged = [this](bool /*isCollapsed*/) {
        resized();  // Re-layout when panel collapses/expands
    };
}

//==============================================================================
void MainComponent::setupProgressPanel()
{
    // Create progress panel - mirrors Python progress_bar setup (lines 1703-1716)
    progressPanel = std::make_unique<ProgressPanel>();
}

//==============================================================================
void MainComponent::setupSplitButton()
{
    // Configure split button as PRIMARY ACTION - large, obvious, impossible to miss
    splitButton.setButtonText("✓ SPLIT FILES");  // Clear, action-oriented text with checkmark
    splitButton.setEnabled(false); // mirrors Python initial state="disabled" (line 1727)

    // Make it visually prominent as primary action
    splitButton.setColour(juce::TextButton::buttonColourId, ModernLookAndFeel::Colors::primary);
    splitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    splitButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    // Set button callback - mirrors Python command=lambda (line 1725)
    splitButton.onClick = [this]() {
        splitButtonClicked();
    };

    // Set tooltip - mirrors Python ToolTip setup (lines 1733-1738)
    splitButton.setTooltip("Split audio file(s) into individual channel files (Cmd+Return)");

    // Set name for custom styling in LookAndFeel
    splitButton.setName("primaryActionButton");
}

//==============================================================================
void MainComponent::setupColors()
{
    // Modern colors are now handled by ModernLookAndFeel automatically
    // This method kept for compatibility but modernLookAndFeel handles all styling
}

//==============================================================================
int MainComponent::getCurrentTab() const
{
    // Mirror Python notebook.select() functionality
    return tabbedComponent.getCurrentTabIndex();
}

//==============================================================================
void MainComponent::setCurrentTab(int tabIndex)
{
    // Mirror Python tab switching
    tabbedComponent.setCurrentTabIndex(tabIndex);
    updateButtonStates();
}

//==============================================================================
void MainComponent::tabChanged()
{
    // Handle tab change - mirrors Python tab change event handling
    updateButtonStates();
}

//==============================================================================
void MainComponent::updateButtonStates()
{
    // Mirror Python update_button_states() function exactly (lines 136-168)
    
    int currentTab = getCurrentTab();
    bool enableSplit = false;
    
    if (currentTab == 0) // "Split Single File" tab
    {
        // Mirror Python single file validation (lines 147-157)
        juce::String singleFilePath = singleFileSplitter->getSelectedFile();
        juce::String outputDir = singleFileSplitter->getOutputDirectory();
        
        if (juce::File(singleFilePath).existsAsFile() && 
            juce::File(outputDir).isDirectory())
        {
            enableSplit = true;
        }
    }
    else if (currentTab == 1) // "Batch Split" tab
    {
        // Mirror Python batch validation (lines 158-168)
        juce::String inputDir = batchSplitter->getInputDirectory();
        juce::String outputDir = batchSplitter->getOutputDirectory();
        
        if (juce::File(inputDir).isDirectory() && 
            juce::File(outputDir).isDirectory())
        {
            enableSplit = true;
        }
    }
    
    // Update button state - mirrors Python button enable/disable logic
    splitButton.setEnabled(enableSplit);
    
    juce::Logger::writeToLog("Button states updated - Split enabled: " + 
                            juce::String(enableSplit ? "true" : "false"));
}

//==============================================================================
void MainComponent::updateFileCount()
{
    // Mirror Python update_file_count() function (lines 474-480)
    if (batchSplitter != nullptr)
    {
        batchSplitter->updateFileCount();
    }
}

//==============================================================================
void MainComponent::splitButtonClicked()
{
    // Mirror Python split_based_on_tab() function (lines 1888-1895, 1917-1924)
    
    int currentTab = getCurrentTab();
    
    if (currentTab == 0) // "Split Single File"
    {
        // Mirror Python single file processing (line 1891-1893)
        singleFileSplitter->startProcessing();
    }
    else if (currentTab == 1) // "Batch Split"
    {
        // Mirror Python batch processing (line 1895)
        batchSplitter->startProcessing();
    }
    
    juce::Logger::writeToLog("Split processing started for tab: " + juce::String(currentTab));
}