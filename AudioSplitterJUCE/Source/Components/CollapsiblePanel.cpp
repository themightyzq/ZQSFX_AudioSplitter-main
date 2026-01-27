#include "CollapsiblePanel.h"

//==============================================================================
CollapsiblePanel::CollapsiblePanel(const juce::String& title,
                                 juce::Component* content,
                                 ConfigManager* config,
                                 const juce::String& key,
                                 bool startCollapsed)
    : contentComponent(content),  // takes ownership via unique_ptr
      configManager(config),
      stateKey(key),
      collapsed(startCollapsed),
      collapseButton("collapse", juce::DrawableButton::ImageOnButtonBackground)
{
    // Setup title label
    titleLabel.setText(title, juce::dontSendNotification);
    titleLabel.setFont(ModernLookAndFeel::Typography::getHeadingFont());
    titleLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

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
void CollapsiblePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw header background
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    g.setColour(ModernLookAndFeel::Colors::surface);
    g.fillRoundedRectangle(headerBounds.toFloat(), 4.0f);

    // Draw header border
    g.setColour(ModernLookAndFeel::Colors::border);
    g.drawRoundedRectangle(headerBounds.toFloat(), 4.0f, 1.0f);

    // Draw content background if expanded
    if (!collapsed && contentComponent)
    {
        g.setColour(ModernLookAndFeel::Colors::background);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        g.setColour(ModernLookAndFeel::Colors::border);
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
    }
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

    // Title label fills remaining header space
    titleLabel.setBounds(headerBounds.reduced(8, 0));

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
    return HEADER_HEIGHT + 8 + contentHeight; // header + spacing + content
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
        collapseButton.setTooltip("Expand panel");
    }
    else
    {
        collapseButton.setImages(expandedIcon.get());
        collapseButton.setTooltip("Collapse panel");
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
