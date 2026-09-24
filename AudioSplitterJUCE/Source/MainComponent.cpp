#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(ConfigManager* config)
    : configManager(config),
      tabbedComponent(juce::TabbedButtonBar::TabsAtTop), // mirrors Python notebook orientation
      splitButton("Split") // mirrors Python split_button text (line 1724)
{
    // The house LookAndFeel is installed as the JUCE default by ScopedHouseLookAndFeel, owned by
    // the application ahead of the window (AudioSplitterApplication) -- this component no longer
    // owns or installs its own.
    logo.onClick = [this]() { showAboutBox(); };
    addAndMakeVisible(logo);

    // Initialize UCS system first
    ucsManager = std::make_unique<UCSManager>();
    if (!ucsManager->loadTaxonomyFromBinaryData())
    {
        // No silent failures (project CLAUDE.md Section H): UCS naming is this product's entire
        // reason to exist, so a failed taxonomy load must reach the user, not just the log.
        // UCSNamingPanel (built below) also reflects this with a persistent inline message; this
        // alert is the immediate, hard-to-miss notice at startup.
        juce::Logger::writeToLog("Warning: Failed to load UCS taxonomy");
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon,
            "UCS Naming Unavailable",
            "ZQ SFX Audio Splitter could not load its embedded UCS taxonomy data.\n\n"
            "UCS naming has been disabled for this session -- files can still be split without "
            "UCS names. If this keeps happening, reinstall the application.",
            "OK");
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

    collapsibleUCSPanel->setContentHeight(UCSNamingPanel::getPreferredContentHeight());
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
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // House chassis gradient (style guide section 6 / Phase 1.5), the same background every
    // ZQ SFX editor/window uses.
    g.setGradientFill(zqsfx::ui::gradients::chassis(getLocalBounds().toFloat()));
    g.fillAll();
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
    const int headerHeight = 28;

    bounds.reduce(margin, margin);

    // Header: the house company mark, far right (style guide section 5 / Phase 1.6). Never
    // under 24px tall.
    auto header = bounds.removeFromTop(headerHeight);
    logo.setBounds(header.removeFromRight(headerHeight).withSizeKeepingCentre(24, 24));
    bounds.removeFromTop(spacing);

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

        // Never shrink a panel below its own collapsed (header-only) height. OptionsPanel lays
        // itself out sequentially from whatever space it is actually given (see
        // OptionsPanel::resized()), so its groups never overlap at any height -- but below this
        // floor the content would be squeezed to an unreadable sliver. Keeping at least the
        // header visible keeps the title and collapse button usable, so a short window degrades
        // to "collapse it yourself" rather than to illegible content.
        ucsHeight = juce::jmax(ucsHeight, collapsibleUCSPanel->getCollapsedHeight());
        optionsHeight = juce::jmax(optionsHeight, collapsibleOptionsPanel->getCollapsedHeight());
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

    // Was a hardcoded 200px, which is less than the ~340px the layout actually needs (margin top
    // + two 150px group rows + inter-row spacing + margin bottom -- see OptionsPanel::resized()),
    // causing the Custom Names group to overlap the row above it. Deriving the height from the
    // same constants the layout uses means the two can never drift apart again.
    collapsibleOptionsPanel->setContentHeight(OptionsPanel::getPreferredContentHeight());
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
    // Configure split button as PRIMARY ACTION - large, obvious, impossible to miss. The house
    // LookAndFeel draws every TextButton the same way (gradient face, hover turns the legend
    // accent) regardless of buttonColourId/textColourOffId/textColourOnId -- see
    // zqsfx::ui::LookAndFeel::drawButtonBackground/drawButtonText, which read only toggle state
    // and enablement, never a per-instance colour -- so this button stays visually prominent
    // through its size and position (bottom, full width, 44px tall) rather than a bespoke colour.
    splitButton.setButtonText("SPLIT FILES");
    splitButton.setEnabled(false); // mirrors Python initial state="disabled" (line 1727)

    // Set button callback - mirrors Python command=lambda (line 1725)
    splitButton.onClick = [this]() {
        splitButtonClicked();
    };

    // Accessibility floor: visible label + tooltip + description (style guide section 8).
    splitButton.setTooltip("Split audio file(s) into individual channel files (Cmd+Return)");
    splitButton.setTitle("Split Files");
    splitButton.setDescription("Split audio file(s) into individual channel files (Cmd+Return)");
}

//==============================================================================
void MainComponent::setupColors()
{
    // Modern colors are now handled by ModernLookAndFeel automatically
    // This method kept for compatibility but modernLookAndFeel handles all styling
}

//==============================================================================
void MainComponent::showAboutBox()
{
    // Standalone app: no JucePlugin_VersionString (that macro only exists for juce_add_plugin
    // targets); JUCEApplication::getApplicationVersion() is the app equivalent, matching
    // DePump's own MainComponent::showAbout() (the worked example for apps). No knob credit
    // line: this product draws no rotary knobs, matching DePump's own About box, which omits it
    // for the same reason.
    auto* app = juce::JUCEApplication::getInstance();
    const juce::String version = app != nullptr ? app->getApplicationVersion() : juce::String();

    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About ZQ SFX Audio Splitter",
        "ZQ SFX Audio Splitter " + version
            + "\n\nZQ SFX - https://www.zq-sfx.com - connect@zq-sfx.com\n"
              "Free software under GPL-3.0-or-later. Built with JUCE.\n"
              "Fonts: Barlow Condensed, VT323, IBM Plex Mono (SIL OFL).",
        "Close",
        this);
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