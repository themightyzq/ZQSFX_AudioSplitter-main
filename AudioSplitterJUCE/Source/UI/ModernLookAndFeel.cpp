#include "ModernLookAndFeel.h"

//==============================================================================
// Color definitions - Modern, accessible palette
const juce::Colour ModernLookAndFeel::Colors::background          = juce::Colour(0xff1a1a1a); // Darker, more modern
const juce::Colour ModernLookAndFeel::Colors::backgroundSecondary = juce::Colour(0xff242424); // Subtle variation
const juce::Colour ModernLookAndFeel::Colors::surface             = juce::Colour(0xff2d2d2d); // Card backgrounds
const juce::Colour ModernLookAndFeel::Colors::surfaceHover        = juce::Colour(0xff353535); // Hover elevation

const juce::Colour ModernLookAndFeel::Colors::textPrimary         = juce::Colour(0xffffffff); // Pure white
const juce::Colour ModernLookAndFeel::Colors::textSecondary       = juce::Colour(0xffb3b3b3); // 70% opacity
const juce::Colour ModernLookAndFeel::Colors::textDisabled        = juce::Colour(0xff666666); // 40% opacity

const juce::Colour ModernLookAndFeel::Colors::primary             = juce::Colour(0xff007acc); // Modern blue
const juce::Colour ModernLookAndFeel::Colors::primaryHover        = juce::Colour(0xff1e88e5); // Lighter blue
const juce::Colour ModernLookAndFeel::Colors::primaryPressed      = juce::Colour(0xff0056b3); // Darker blue
const juce::Colour ModernLookAndFeel::Colors::secondary           = juce::Colour(0xff6c757d); // Neutral gray

const juce::Colour ModernLookAndFeel::Colors::success             = juce::Colour(0xff28a745); // Green
const juce::Colour ModernLookAndFeel::Colors::warning             = juce::Colour(0xffffc107); // Amber
const juce::Colour ModernLookAndFeel::Colors::error               = juce::Colour(0xffdc3545); // Red
const juce::Colour ModernLookAndFeel::Colors::info                = juce::Colour(0xff17a2b8); // Cyan

const juce::Colour ModernLookAndFeel::Colors::border              = juce::Colour(0xff404040); // Subtle borders
const juce::Colour ModernLookAndFeel::Colors::borderHover         = juce::Colour(0xff555555); // Hover borders
const juce::Colour ModernLookAndFeel::Colors::borderFocus         = juce::Colour(0xff007acc); // Focus rings
const juce::Colour ModernLookAndFeel::Colors::outline             = juce::Colour(0xffffffff); // High contrast outline

//==============================================================================
ModernLookAndFeel::ModernLookAndFeel()
{
    // Set default component colors
    setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
    
    // Button colors
    setColour(juce::TextButton::buttonColourId, Colors::surface);
    setColour(juce::TextButton::buttonOnColourId, Colors::primary);
    setColour(juce::TextButton::textColourOffId, Colors::textPrimary);
    setColour(juce::TextButton::textColourOnId, Colors::textPrimary);
    
    // Text editor colors
    setColour(juce::TextEditor::backgroundColourId, Colors::surface);
    setColour(juce::TextEditor::textColourId, Colors::textPrimary);
    setColour(juce::TextEditor::highlightColourId, Colors::primary.withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, Colors::textPrimary);
    setColour(juce::TextEditor::outlineColourId, Colors::border);
    setColour(juce::TextEditor::focusedOutlineColourId, Colors::borderFocus);
    
    // Label colors
    setColour(juce::Label::textColourId, Colors::textPrimary);
    
    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, Colors::surface);
    setColour(juce::ComboBox::textColourId, Colors::textPrimary);
    setColour(juce::ComboBox::outlineColourId, Colors::border);
    setColour(juce::ComboBox::arrowColourId, Colors::textPrimary);
    setColour(juce::ComboBox::focusedOutlineColourId, Colors::borderFocus);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    // Group component colors
    setColour(juce::GroupComponent::outlineColourId, Colors::border);
    setColour(juce::GroupComponent::textColourId, Colors::textSecondary);
    
    // Tabbed component colors
    setColour(juce::TabbedComponent::backgroundColourId, Colors::background);
    setColour(juce::TabbedComponent::outlineColourId, Colors::border);
    setColour(juce::TabbedButtonBar::tabOutlineColourId, Colors::border);
    setColour(juce::TabbedButtonBar::tabTextColourId, Colors::textSecondary);
    setColour(juce::TabbedButtonBar::frontOutlineColourId, Colors::primary);
    setColour(juce::TabbedButtonBar::frontTextColourId, Colors::textPrimary);
    
    // Progress bar colors
    setColour(juce::ProgressBar::backgroundColourId, Colors::surface);
    setColour(juce::ProgressBar::foregroundColourId, Colors::primary);
    
    // Toggle button colors
    setColour(juce::ToggleButton::textColourId, Colors::textPrimary);
    setColour(juce::ToggleButton::tickColourId, Colors::primary);
    setColour(juce::ToggleButton::tickDisabledColourId, Colors::textDisabled);
}

//==============================================================================
ModernLookAndFeel::~ModernLookAndFeel()
{
}

//==============================================================================
void ModernLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerRadius = 6.0f; // Modern rounded corners

    // PRIMARY ACTION BUTTONS: Enhanced styling for prominence
    bool isPrimaryAction = button.getName() == "primaryActionButton";
    if (isPrimaryAction)
    {
        cornerRadius = 8.0f;  // Larger radius for primary buttons
    }

    auto buttonColour = getButtonColour(button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    // Draw shadow for elevation
    if (button.isEnabled())
    {
        if (isPrimaryAction)
        {
            // Larger, more prominent shadow for primary action
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(bounds.translated(0, 2), cornerRadius);
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.1f));
            g.fillRoundedRectangle(bounds.translated(0, 1), cornerRadius);
        }
    }

    // Draw main button
    drawRoundedRectangle(g, bounds, cornerRadius, buttonColour, Colors::border, 1.0f);

    // Draw focus ring if needed
    if (button.hasKeyboardFocus(true))
    {
        drawFocusRing(g, bounds.toNearestInt(), isPrimaryAction ? 3 : 2);
    }
}

//==============================================================================
void ModernLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    // PRIMARY ACTION BUTTONS: Use larger, bolder font
    bool isPrimaryAction = button.getName() == "primaryActionButton";
    auto font = isPrimaryAction ? Typography::getHeadingFont() : Typography::getBodyFont();
    g.setFont(font);

    auto textColour = button.findColour(juce::TextButton::textColourOffId);
    if (!button.isEnabled())
        textColour = Colors::textDisabled;
    else if (shouldDrawButtonAsDown || button.getToggleState())
        textColour = button.findColour(juce::TextButton::textColourOnId);

    g.setColour(textColour);
    
    auto bounds = button.getLocalBounds();
    g.drawFittedText(button.getButtonText(), bounds, juce::Justification::centred, 1);
}

//==============================================================================
void ModernLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                juce::TextEditor& textEditor)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    auto cornerRadius = 4.0f;
    
    auto backgroundColour = textEditor.findColour(juce::TextEditor::backgroundColourId);
    if (textEditor.hasKeyboardFocus(true))
        backgroundColour = backgroundColour.brighter(0.05f);
        
    drawRoundedRectangle(g, bounds, cornerRadius, backgroundColour);
}

//==============================================================================
void ModernLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                             juce::TextEditor& textEditor)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    auto cornerRadius = 4.0f;
    
    auto outlineColour = Colors::border;
    if (textEditor.hasKeyboardFocus(true))
        outlineColour = Colors::borderFocus;
    else if (textEditor.isMouseOver())
        outlineColour = Colors::borderHover;
        
    drawRoundedRectangle(g, bounds, cornerRadius, juce::Colour(), outlineColour, 1.0f);
}

//==============================================================================
void ModernLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto bounds = label.getLocalBounds();
    
    // Background
    auto backgroundColour = label.findColour(juce::Label::backgroundColourId);
    if (!backgroundColour.isTransparent())
    {
        g.setColour(backgroundColour);
        g.fillRect(bounds);
    }
    
    // Text -- respect the label's own font if explicitly set, otherwise use design system default
    auto font = label.getFont();
    if (font.getHeight() < 1.0f)
        font = Typography::getBodyFont();
    g.setFont(font);

    auto textColour = label.findColour(juce::Label::textColourId);
    if (!label.isEnabled())
        textColour = Colors::textDisabled;

    g.setColour(textColour);

    auto textBounds = bounds.reduced(Spacing::xs);
    g.drawFittedText(label.getText(), textBounds, label.getJustificationType(), 1);
}

//==============================================================================
void ModernLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                                 const juce::String& text,
                                                 const juce::Justification& position,
                                                 juce::GroupComponent& group)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    auto cornerRadius = 8.0f;
    auto headerHeight = 24.0f;
    
    // Draw background
    drawRoundedRectangle(g, bounds, cornerRadius, Colors::surface, Colors::border, 1.0f);
    
    // Draw header
    if (text.isNotEmpty())
    {
        auto headerBounds = bounds.removeFromTop(headerHeight);
        g.setColour(Colors::backgroundSecondary);
        g.fillRoundedRectangle(headerBounds.getX(), headerBounds.getY(), 
                              headerBounds.getWidth(), headerBounds.getHeight(),
                              cornerRadius);
        
        // Draw text
        g.setFont(Typography::getSubheadingFont());
        g.setColour(group.findColour(juce::GroupComponent::textColourId));
        g.drawText(text, headerBounds.reduced(Spacing::md, 0), juce::Justification::centredLeft);
    }
}

//==============================================================================
void ModernLookAndFeel::drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                                     bool isMouseOver, bool isMouseDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerRadius = 6.0f;
    
    // Background
    juce::Colour backgroundColour = Colors::background;
    if (button.isFrontTab())
        backgroundColour = Colors::surface;
    else if (isMouseOver)
        backgroundColour = Colors::surfaceHover;
        
    if (button.isFrontTab())
    {
        // Active tab - rounded top corners only
        g.setColour(backgroundColour);
        juce::Path path;
        path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight() + cornerRadius,
                               cornerRadius, cornerRadius, true, true, false, false);
        g.fillPath(path);
        
        // Active indicator
        g.setColour(Colors::primary);
        g.fillRect(juce::Rectangle<float>(bounds.getX(), bounds.getBottom() - 2, bounds.getWidth(), 2));
    }
    else
    {
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
    
    // Text
    auto font = Typography::getBodyFont();
    if (button.isFrontTab())
        font = Typography::getSubheadingFont();
        
    g.setFont(font);
    
    auto textColour = button.isFrontTab() ? Colors::textPrimary : Colors::textSecondary;
    if (!button.isEnabled())
        textColour = Colors::textDisabled;
        
    g.setColour(textColour);
    
    auto textBounds = bounds.reduced(Spacing::sm, Spacing::xs).toNearestInt();
    g.drawFittedText(button.getButtonText(), textBounds, juce::Justification::centred, 1);

    // Focus ring
    if (button.hasKeyboardFocus(true))
    {
        drawFocusRing(g, bounds.toNearestInt(), 2);
    }
}

//==============================================================================
int ModernLookAndFeel::getTabButtonBestWidth(juce::TabBarButton& button, int /*tabDepth*/)
{
    // Use the larger font (subheading) for width calculation to prevent truncation
    auto font = Typography::getSubheadingFont();
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, button.getButtonText(), 0.0f, 0.0f);
    int textWidth = (int)std::ceil(glyphs.getBoundingBox(0, -1, false).getWidth());
    return textWidth + Spacing::lg * 2; // 24px padding each side
}

//==============================================================================
void ModernLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar& bar, juce::Graphics& g)
{
    auto bounds = bar.getLocalBounds();
    g.setColour(Colors::background);
    g.fillRect(bounds);
    
    // Bottom border
    g.setColour(Colors::border);
    g.fillRect(juce::Rectangle<int>(bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1));
}

//==============================================================================
void ModernLookAndFeel::drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar,
                                       int width, int height, double progress,
                                       const juce::String& textToShow)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    auto cornerRadius = 4.0f;
    
    // Background
    drawRoundedRectangle(g, bounds, cornerRadius, Colors::surface, Colors::border, 1.0f);
    
    // Progress fill
    if (progress >= 0.0)
    {
        auto progressWidth = bounds.getWidth() * (float)progress;
        auto progressBounds = bounds.withWidth(progressWidth);
        
        drawRoundedRectangle(g, progressBounds, cornerRadius, Colors::primary);
    }
    
    // Text
    if (textToShow.isNotEmpty())
    {
        g.setFont(Typography::getSmallFont());
        g.setColour(Colors::textPrimary);
        g.drawText(textToShow, bounds.toNearestInt(), juce::Justification::centred);
    }
}

//==============================================================================
void ModernLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds();
    auto checkboxSize = 16.0f;
    auto checkboxBounds = juce::Rectangle<float>(0, 0, checkboxSize, checkboxSize)
                         .withCentre(juce::Point<float>(checkboxSize * 0.5f + Spacing::xs, 
                                                       bounds.getCentreY()));
    
    // Checkbox background
    auto cornerRadius = 3.0f;
    auto backgroundColour = Colors::surface;
    auto borderColour = Colors::border;
    
    if (button.getToggleState())
    {
        backgroundColour = Colors::primary;
        borderColour = Colors::primary;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        backgroundColour = Colors::surfaceHover;
        borderColour = Colors::borderHover;
    }
    
    if (!button.isEnabled())
    {
        backgroundColour = Colors::surface.withAlpha(0.5f);
        borderColour = Colors::textDisabled;
    }
    
    drawRoundedRectangle(g, checkboxBounds, cornerRadius, backgroundColour, borderColour, 1.0f);
    
    // Checkmark
    if (button.getToggleState())
    {
        g.setColour(button.isEnabled() ? Colors::textPrimary : Colors::textDisabled);
        
        juce::Path checkmark;
        checkmark.startNewSubPath(checkboxBounds.getX() + 3, checkboxBounds.getCentreY());
        checkmark.lineTo(checkboxBounds.getCentreX() - 1, checkboxBounds.getBottom() - 4);
        checkmark.lineTo(checkboxBounds.getRight() - 3, checkboxBounds.getY() + 4);
        
        g.strokePath(checkmark, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, 
                                                    juce::PathStrokeType::rounded));
    }
    
    // Text
    auto textBounds = bounds.withTrimmedLeft((int)(checkboxSize + Spacing::sm + Spacing::xs));
    if (!textBounds.isEmpty())
    {
        g.setFont(Typography::getBodyFont());
        
        auto textColour = Colors::textPrimary;
        if (!button.isEnabled())
            textColour = Colors::textDisabled;
            
        g.setColour(textColour);
        g.drawFittedText(button.getButtonText(), textBounds, juce::Justification::centredLeft, 1);
    }
    
    // Focus ring
    if (button.hasKeyboardFocus(true))
    {
        drawFocusRing(g, checkboxBounds.expanded(2).toNearestInt(), 2);
    }
}

//==============================================================================
void ModernLookAndFeel::drawFocusRing(juce::Graphics& g, const juce::Rectangle<int>& bounds, int thickness)
{
    g.setColour(Colors::borderFocus);
    g.drawRoundedRectangle(bounds.toFloat().reduced(thickness * 0.5f), 6.0f, static_cast<float>(thickness));
}

//==============================================================================
void ModernLookAndFeel::drawDropZoneHint(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                         const juce::String& hintText)
{
    auto area = bounds.reduced(Spacing::xl);

    g.setColour(Colors::border);
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.5f);

    g.setColour(Colors::textSecondary);
    g.setFont(Typography::getSubheadingFont());
    g.drawText(hintText, area, juce::Justification::centred, true);
}

//==============================================================================
void ModernLookAndFeel::drawRoundedRectangle(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                           float cornerRadius, const juce::Colour& fillColour,
                                           const juce::Colour& borderColour, float borderThickness)
{
    if (!fillColour.isTransparent())
    {
        g.setColour(fillColour);
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
    
    if (!borderColour.isTransparent() && borderThickness > 0.0f)
    {
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5f), cornerRadius, borderThickness);
    }
}

//==============================================================================
juce::Colour ModernLookAndFeel::getButtonColour(juce::Button& button, bool isHighlighted, bool isDown)
{
    if (!button.isEnabled())
        return Colors::surface.withAlpha(0.5f);
        
    if (isDown || button.getToggleState())
        return Colors::primaryPressed;
    else if (isHighlighted)
        return Colors::primaryHover;
    else
        return Colors::surface;
}

//==============================================================================
void ModernLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    auto cornerSize = 6.0f;
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    
    // Background
    auto fillColour = box.isEnabled() ? Colors::surface : Colors::surface.withAlpha(0.5f);
    if (box.hasKeyboardFocus(true))
        fillColour = Colors::surfaceHover;
    
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    // Border
    auto borderColour = Colors::border;
    if (box.hasKeyboardFocus(true))
        borderColour = Colors::borderFocus;
    else if (box.isMouseOver())
        borderColour = Colors::borderHover;
        
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    
    // Arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX + 4.0f, buttonY + 4.0f, 
                                              buttonW - 8.0f, buttonH - 8.0f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                      arrowBounds.getCentreX(), arrowBounds.getBottom(),
                      arrowBounds.getRight(), arrowBounds.getY());
    
    g.setColour(box.isEnabled() ? Colors::textPrimary : Colors::textDisabled);
    g.fillPath(arrow);
}

juce::Label* ModernLookAndFeel::createComboBoxTextBox(juce::ComboBox& box)
{
    auto* label = LookAndFeel_V4::createComboBoxTextBox(box);
    
    label->setColour(juce::Label::textColourId, Colors::textPrimary);
    label->setColour(juce::Label::textWhenEditingColourId, Colors::textPrimary);
    label->setFont(Typography::getBodyFont());
    label->setJustificationType(juce::Justification::centredLeft);
    
    return label;
}