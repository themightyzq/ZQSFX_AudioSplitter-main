#include "MainComponent.h"

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

    collapsibleUCSPanel->setContentHeight(250);
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
    
    // Enable keyboard focus so Cmd+Return shortcut works
    setWantsKeyboardFocus(true);

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

    const int margin = ModernLookAndFeel::Spacing::md;   // 16px
    const int spacing = ModernLookAndFeel::Spacing::sm;   // 8px
    const int buttonHeight = 44;
    const int progressHeight = 60;
    const int minTabHeight = 200;  // Tabs must always be at least this tall

    bounds.reduce(margin, margin);

    // Fixed bottom elements: split button + progress panel
    splitButton.setBounds(bounds.removeFromBottom(buttonHeight).reduced(spacing, 0));
    bounds.removeFromBottom(spacing);

    progressPanel->setBounds(bounds.removeFromBottom(progressHeight));
    bounds.removeFromBottom(spacing);

    // Collapsible panels: proportional scaling if they don't fit
    int idealUCS = collapsibleUCSPanel->getIdealHeight();
    int idealOptions = collapsibleOptionsPanel->getIdealHeight();
    int totalPanelIdeal = idealUCS + idealOptions + spacing;
    int availableForPanels = bounds.getHeight() - minTabHeight - spacing * 2;

    int ucsHeight, optionsHeight;
    if (totalPanelIdeal <= availableForPanels)
    {
        // Panels fit at ideal size
        ucsHeight = idealUCS;
        optionsHeight = idealOptions;
    }
    else
    {
        // Scale panels down proportionally to guarantee minimum tab height
        float scale = juce::jmax(0.3f, (float)availableForPanels / (float)juce::jmax(1, totalPanelIdeal));
        ucsHeight = (int)(idealUCS * scale);
        optionsHeight = (int)(idealOptions * scale);
    }

    collapsibleUCSPanel->setBounds(bounds.removeFromBottom(ucsHeight));
    bounds.removeFromBottom(spacing);

    collapsibleOptionsPanel->setBounds(bounds.removeFromBottom(optionsHeight));
    bounds.removeFromBottom(spacing);

    // Tabs take remaining space
    tabbedComponent.setBounds(bounds);
}

//==============================================================================
bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Cmd+Return (macOS) / Ctrl+Return (Windows) triggers the split action
    if (key == juce::KeyPress(juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier, 0))
    {
        if (splitButton.isEnabled())
            splitButtonClicked();
        return true;
    }
    return false;
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
        "Single File",              // concise tab label
        ModernLookAndFeel::Colors::background,
        singleFileSplitter.get(),
        false
    );

    tabbedComponent.addTab(
        "Batch",                    // concise tab label
        ModernLookAndFeel::Colors::background,  
        batchSplitter.get(),        // tab content
        false                       // don't delete on removal
    );
    
    // Set tab change callback - mirrors Python tab selection handling
    tabbedComponent.setTabBarDepth(36); // tall enough for readable text
    
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

    collapsibleOptionsPanel->setContentHeight(200);
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
    splitButton.setButtonText("SPLIT FILES");
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
    
    // Update button state and tooltip
    splitButton.setEnabled(enableSplit);

    if (enableSplit)
    {
        splitButton.setTooltip("Split audio file(s) into individual channel files (Cmd+Return)");
    }
    else
    {
        // Explain why the button is disabled
        if (currentTab == 0)
        {
            juce::String singleFilePath = singleFileSplitter->getSelectedFile();
            if (singleFilePath.isEmpty() || !juce::File(singleFilePath).existsAsFile())
                splitButton.setTooltip("Select a valid WAV file to enable splitting");
            else
                splitButton.setTooltip("Select a valid output directory to enable splitting");
        }
        else
        {
            juce::String inputDir = batchSplitter->getInputDirectory();
            if (inputDir.isEmpty() || !juce::File(inputDir).isDirectory())
                splitButton.setTooltip("Select a valid input directory to enable splitting");
            else
                splitButton.setTooltip("Select a valid output directory to enable splitting");
        }
    }

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
    
    // Disable button to prevent double-click launching concurrent processes
    splitButton.setEnabled(false);

    int currentTab = getCurrentTab();

    if (currentTab == 0) // "Split Single File"
    {
        singleFileSplitter->startProcessing();
    }
    else if (currentTab == 1) // "Batch Split"
    {
        batchSplitter->startProcessing();
    }

    juce::Logger::writeToLog("Split processing started for tab: " + juce::String(currentTab));

    // Re-enable after a short delay (processing callbacks will manage final state)
    auto weak = juce::Component::SafePointer<MainComponent>(this);
    juce::Timer::callAfterDelay(1000, [weak]()
    {
        if (auto* self = weak.getComponent())
            self->updateButtonStates();
    });
}