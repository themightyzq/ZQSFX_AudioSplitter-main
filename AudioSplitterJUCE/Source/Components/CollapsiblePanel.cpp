#include "CollapsiblePanel.h"

//==============================================================================
CollapsiblePanel::CollapsiblePanel(const juce::String& title,
                                 juce::Component* content,
                                 ConfigManager* config,
                                 const juce::String& key,
                                 bool startCollapsed)
    : panelTitle(title),
      collapseButton("collapse", juce::DrawableButton::ImageOnButtonBackground),
      contentComponent(content),  // takes ownership via unique_ptr
      collapsed(startCollapsed),
      configManager(config),
      stateKey(key)
{
    // Accessibility floor: the title used to be an accessible juce::Label; now that it is
    // hand-drawn (so it can use the house's platform-bold title face -- see paint() below), the
    // panel itself needs to announce it explicitly.
    setTitle(panelTitle);
    setDescription(panelTitle + " panel");
    setAccessible(true);

    // Create icons for collapse button
    createIcons();

    // Setup collapse button
    collapseButton.setClickingTogglesState(false);
    collapseButton.onClick = [this]() { toggleCollapsed(); };
    updateCollapseButton();
    addAndMakeVisible(collapseButton);

    // Add content component
    if (contentComponent)
    {
        addAndMakeVisible(contentComponent.get());
        contentHeight = contentComponent->getHeight();
        if (contentHeight < MIN_CONTENT_HEIGHT)
            contentHeight = MIN_CONTENT_HEIGHT;
    }

    // Load saved state
    loadState();

    // Set initial visibility
    if (contentComponent)
        contentComponent->setVisible(!collapsed);

    // Set initial size
    setSize(400, getIdealHeight());
}

//==============================================================================
CollapsiblePanel::~CollapsiblePanel()
{
    saveState();
}

//==============================================================================
// Drawn like zqsfx::ui::Panel (gradient face, hard 1px border, platform-bold wide-tracked title,
// ruleTitle hairline under it) while keeping this component's own collapse behaviour and title
// label/collapse-button children untouched. No rounded corners (style guide section 6).
void CollapsiblePanel::paint(juce::Graphics& g)
{
    namespace colour = zqsfx::ui::colour;

    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(zqsfx::ui::gradients::panel(bounds));
    g.fillRect(bounds);
    g.setColour(juce::Colours::white.withAlpha(0.04f)); // inner top highlight
    g.fillRect(bounds.withHeight(1.0f).translated(0.0f, 1.0f));
    g.setColour(colour::panelBorder);
    g.drawRect(bounds, 1.0f);

    g.setColour(colour::ruleTitle);
    g.fillRect(juce::Rectangle<float>(0.0f, (float) HEADER_HEIGHT - 1.0f, bounds.getWidth(), 1.0f));

    // Title: platform bold, wide-tracked, silkTitle -- the house's own section-title treatment
    // (zqsfx::ui::Panel::paint), drawn directly rather than through a Label so the platform-bold
    // face is used instead of the LookAndFeel's silk (Barlow Condensed) face every Label renders
    // through.
    auto headerBounds = getLocalBounds().removeFromTop(HEADER_HEIGHT).removeFromLeft(getWidth() - HEADER_HEIGHT);
    g.setColour(colour::silkTitle);
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)).withExtraKerningFactor(0.27f));
    g.drawText(panelTitle.toUpperCase(), headerBounds.reduced(8, 0), juce::Justification::centredLeft);
}

//==============================================================================
void CollapsiblePanel::resized()
{
    auto bounds = getLocalBounds();

    // Header area
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);

    // Collapse button on the right side of header
    auto buttonBounds = headerBounds.removeFromRight(HEADER_HEIGHT);
    buttonBounds = buttonBounds.reduced(4);
    collapseButton.setBounds(buttonBounds);

    // Title is drawn directly in paint() over the remaining header space; nothing to lay out.

    // Content area (if expanded)
    if (!collapsed && contentComponent)
    {
        bounds.removeFromTop(4); // spacing
        contentComponent->setBounds(bounds.reduced(4));
    }
}

//==============================================================================
void CollapsiblePanel::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is in header area
    if (event.y < HEADER_HEIGHT)
    {
        toggleCollapsed();
    }
}

//==============================================================================
void CollapsiblePanel::setCollapsed(bool shouldBeCollapsed, bool animate)
{
    if (collapsed == shouldBeCollapsed)
        return;

    collapsed = shouldBeCollapsed;

    // Update button icon
    updateCollapseButton();

    // Show/hide content
    if (contentComponent)
    {
        if (animate)
        {
            // Animate content visibility
            if (collapsed)
            {
                animator.fadeOut(contentComponent.get(), 150);
            }
            else
            {
                contentComponent->setVisible(true);
                animator.fadeIn(contentComponent.get(), 150);
            }
        }
        else
        {
            contentComponent->setVisible(!collapsed);
        }
    }

    // Notify parent to adjust layout
    if (onCollapseChanged)
        onCollapseChanged(collapsed);

    // Save state
    saveState();

    // Trigger repaint
    repaint();
}

//==============================================================================
void CollapsiblePanel::toggleCollapsed()
{
    setCollapsed(!collapsed, true);
}

//==============================================================================
int CollapsiblePanel::getExpandedHeight() const
{
    // header + 4 px spacing + content inset 4 px top and bottom (see resized()): 12, not 8.
    // At 8 the content component was handed 4 px less than it asked for, which clipped the
    // bottom row of every wrapped panel (the UCS preview line was the visible case).
    return HEADER_HEIGHT + 12 + contentHeight;
}

//==============================================================================
int CollapsiblePanel::getCollapsedHeight() const
{
    return HEADER_HEIGHT;
}

//==============================================================================
int CollapsiblePanel::getIdealHeight() const
{
    return collapsed ? getCollapsedHeight() : getExpandedHeight();
}

//==============================================================================
void CollapsiblePanel::setContentHeight(int height)
{
    contentHeight = juce::jmax(MIN_CONTENT_HEIGHT, height);
}

//==============================================================================
void CollapsiblePanel::createIcons()
{
    // Create expanded icon (▼ down arrow)
    {
        juce::Path path;
        path.addTriangle(0.0f, 0.0f, 10.0f, 0.0f, 5.0f, 6.0f);

        expandedIcon = std::make_unique<juce::DrawablePath>();
        dynamic_cast<juce::DrawablePath*>(expandedIcon.get())->setPath(path);
        dynamic_cast<juce::DrawablePath*>(expandedIcon.get())->setFill(ModernLookAndFeel::Colors::textPrimary);
    }

    // Create collapsed icon (▶ right arrow)
    {
        juce::Path path;
        path.addTriangle(0.0f, 0.0f, 0.0f, 10.0f, 6.0f, 5.0f);

        collapsedIcon = std::make_unique<juce::DrawablePath>();
        dynamic_cast<juce::DrawablePath*>(collapsedIcon.get())->setPath(path);
        dynamic_cast<juce::DrawablePath*>(collapsedIcon.get())->setFill(ModernLookAndFeel::Colors::textPrimary);
    }
}

//==============================================================================
void CollapsiblePanel::updateCollapseButton()
{
    if (collapsed)
    {
        collapseButton.setImages(collapsedIcon.get());
        collapseButton.setTooltip("Expand " + panelTitle + " panel");
        collapseButton.setTitle("Expand " + panelTitle);
        collapseButton.setDescription("Expand the " + panelTitle + " panel");
    }
    else
    {
        collapseButton.setImages(expandedIcon.get());
        collapseButton.setTooltip("Collapse " + panelTitle + " panel");
        collapseButton.setTitle("Collapse " + panelTitle);
        collapseButton.setDescription("Collapse the " + panelTitle + " panel");
    }
}

//==============================================================================
void CollapsiblePanel::saveState()
{
    if (configManager && !stateKey.isEmpty())
    {
        configManager->setUIState(stateKey, collapsed ? "1" : "0");
    }
}

//==============================================================================
void CollapsiblePanel::loadState()
{
    if (configManager && !stateKey.isEmpty())
    {
        auto savedState = configManager->getUIState(stateKey);
        if (savedState.isNotEmpty())
        {
            collapsed = (savedState == "1");
        }
    }
}
