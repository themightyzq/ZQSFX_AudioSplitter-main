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

    // Add components to layout: the scrolling body, then the fixed bottom bar.
    body.addAndMakeVisible(tabbedComponent);
    body.addAndMakeVisible(*collapsibleOptionsPanel);
    body.addAndMakeVisible(*collapsibleUCSPanel);
    bodyViewport.setViewedComponent(&body, false);
    bodyViewport.setScrollBarsShown(true, false);
    bodyViewport.setScrollBarThickness(12);
    bodyViewport.setTitle("Settings");
    addAndMakeVisible(bodyViewport);
    addAndMakeVisible(*progressPanel);
    addAndMakeVisible(splitButton);
    
    // Don't set size here - let the parent window control the size
    // The MainWindow will set the appropriate size
    
    // Enable keyboard focus so Cmd+Return shortcut works
    setWantsKeyboardFocus(true);

    // Only now: the tab callbacks and state updates need both splitters and the UCS panel.
    tabbedComponent.onTabChanged = [this]() { tabChanged(); };

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
namespace
{
    constexpr int kSideBySideMinWidth = 900;  // Options and UCS Naming share a row from this width up
    constexpr int kTabBarDepth = 36;
    constexpr int kTabContentSlack = 6;       // TabbedComponent's own outline and indent
    constexpr int kBottomBarHeight = 44;
}

int MainComponent::getTabsPreferredHeight(int width) const
{
    const int contentWidth = width - 2; // inside the tab outline
    const int content = getCurrentTab() == 0 ? singleFileSplitter->getPreferredHeight(contentWidth)
                                             : batchSplitter->getPreferredHeight();
    return kTabBarDepth + content + kTabContentSlack;
}

int MainComponent::getPanelsPreferredHeight(int width) const
{
    const int optionsHeight = collapsibleOptionsPanel->getIdealHeight();
    const int ucsHeight = collapsibleUCSPanel->getIdealHeight();

    if (width >= kSideBySideMinWidth)
        return juce::jmax(optionsHeight, ucsHeight);

    return optionsHeight + ModernLookAndFeel::Spacing::sm + ucsHeight;
}

void MainComponent::layoutBody()
{
    const int gap = ModernLookAndFeel::Spacing::sm;
    const int viewportHeight = bodyViewport.getHeight();

    // Tabs at their preferred height, then the panels. If that is taller than the viewport the
    // column scrolls (and loses the scroll bar's width); otherwise the tabs take the spare room.
    int width = bodyViewport.getWidth();
    auto neededAt = [&](int w) { return getTabsPreferredHeight(w) + gap + getPanelsPreferredHeight(w); };

    if (neededAt(width) > viewportHeight)
        width -= bodyViewport.getScrollBarThickness();

    const int panelsHeight = getPanelsPreferredHeight(width);
    const int height = juce::jmax(neededAt(width), viewportHeight);
    body.setBounds(0, 0, width, height);

    const int tabsHeight = height - panelsHeight - gap;
    tabbedComponent.setBounds(0, 0, width, tabsHeight);

    const int panelsTop = tabsHeight + gap;
    const int optionsHeight = collapsibleOptionsPanel->getIdealHeight();
    const int ucsHeight = collapsibleUCSPanel->getIdealHeight();

    if (width >= kSideBySideMinWidth)
    {
        const int optionsWidth = (width - gap) * 55 / 100;
        collapsibleOptionsPanel->setBounds(0, panelsTop, optionsWidth, optionsHeight);
        collapsibleUCSPanel->setBounds(optionsWidth + gap, panelsTop, width - optionsWidth - gap, ucsHeight);
    }
    else
    {
        collapsibleOptionsPanel->setBounds(0, panelsTop, width, optionsHeight);
        collapsibleUCSPanel->setBounds(0, panelsTop + optionsHeight + gap, width, ucsHeight);
    }
}

void MainComponent::resized()
{
    // Early return if components aren't initialized yet
    if (!collapsibleOptionsPanel || !progressPanel || !collapsibleUCSPanel
        || !singleFileSplitter || !batchSplitter)
        return;

    const int margin = ModernLookAndFeel::Spacing::md - 4;   // 12px sides and top
    const int spacing = ModernLookAndFeel::Spacing::sm;      // 8px
    const int headerHeight = 28;

    auto bounds = getLocalBounds().reduced(margin, margin);

    // Header: the house company mark, far right (style guide section 5 / Phase 1.6). Never
    // under 24px tall.
    auto header = bounds.removeFromTop(headerHeight);
    logo.setBounds(header.removeFromRight(headerHeight).withSizeKeepingCentre(24, 24));
    bounds.removeFromTop(spacing / 2);

    // Fixed bottom bar: Split, then the progress panel at the height its current mode needs.
    splitButton.setBounds(bounds.removeFromBottom(kBottomBarHeight).reduced(spacing, 0));
    bounds.removeFromBottom(spacing);

    progressPanel->setBounds(bounds.removeFromBottom(progressPanel->getPreferredHeight()));
    bounds.removeFromBottom(spacing);

    // Everything else scrolls as one column.
    bodyViewport.setBounds(bounds);
    layoutBody();
}

//==============================================================================
void MainComponent::focusOfChildComponentChanged(FocusChangeType)
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == nullptr || !body.isParentOf(focused))
        return;

    const auto target = body.getLocalArea(focused, focused->getLocalBounds());
    const auto view = bodyViewport.getViewArea();
    int y = view.getY();

    if (target.getY() < view.getY())
        y = target.getY() - 4;
    else if (target.getBottom() > view.getBottom())
        y = target.getBottom() - view.getHeight() + 4;

    if (y != view.getY())
        bodyViewport.setViewPosition(view.getX(), juce::jmax(0, y));
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

    // Split stays disabled for exactly as long as a run is going (see updateButtonStates).
    singleFileSplitter->onBusyChanged = [this]() { updateButtonStates(); };
    singleFileSplitter->onPreferredHeightChanged = [this]() { resized(); };
    batchSplitter->onBusyChanged = [this]() { updateButtonStates(); };
    
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
    tabbedComponent.setTabBarDepth(kTabBarDepth); // tall enough for readable text
    
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
    progressPanel->onCancelRequested = [this]() { cancelRunningOperation(); };
    progressPanel->onPreferredHeightChanged = [this]() { resized(); };
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
    if (ucsNamingPanelContent != nullptr)
        ucsNamingPanelContent->setBatchTabActive(getCurrentTab() == 1);

    updateButtonStates();
    resized(); // each tab needs a different height
}

//==============================================================================
void MainComponent::updateButtonStates()
{
    // Mirror Python update_button_states() function exactly (lines 136-168)
    
    if (singleFileSplitter == nullptr || batchSplitter == nullptr)
        return;

    // A run in progress owns the button: it comes back when the run completes or is cancelled,
    // not on a timer. Options and tab changes during a run must not re-enable it either.
    if (isSplitRunning())
    {
        splitButton.setEnabled(false);
        splitButton.setTooltip("A split is running. Use Cancel next to the progress bar to stop it.");
        return;
    }

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
        
        // The folder is read on a background thread; Split waits for the result.
        if (juce::File(inputDir).isDirectory() &&
            juce::File(outputDir).isDirectory() &&
            !batchSplitter->isScanning())
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
        else if (batchSplitter->isScanning())
        {
            splitButton.setTooltip("Reading the input folder... Split is available as soon as it finishes");
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
bool MainComponent::isSplitRunning() const
{
    return (singleFileSplitter != nullptr && singleFileSplitter->isBusy())
        || (batchSplitter != nullptr && batchSplitter->isBusy());
}

//==============================================================================
void MainComponent::cancelRunningOperation()
{
    if (singleFileSplitter != nullptr && singleFileSplitter->isBusy())
        singleFileSplitter->cancelProcessing();
    else if (batchSplitter != nullptr && batchSplitter->isBusy())
        batchSplitter->cancelProcessing();
    else
        return; // nothing running: a stale click must not leave the panel stuck on "Cancelling"

    progressPanel->setCancelPending();
    juce::Logger::writeToLog("Cancel requested by user");
}

//==============================================================================
void MainComponent::splitButtonClicked()
{
    // Mirror Python split_based_on_tab() function (lines 1888-1895, 1917-1924)
    const int currentTab = getCurrentTab();

    if (currentTab == 0) // "Split Single File"
    {
        singleFileSplitter->startProcessing();
    }
    else if (currentTab == 1) // "Batch Split"
    {
        batchSplitter->startProcessing();
    }

    juce::Logger::writeToLog("Split processing requested for tab: " + juce::String(currentTab));

    // Disabled while the run it started is going; unchanged (still enabled) if validation
    // stopped it before anything ran. The run's completion re-evaluates this too.
    updateButtonStates();
}
