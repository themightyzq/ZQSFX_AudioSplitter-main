#include "ModernLookAndFeel.h"

//==============================================================================
// Color definitions. Every name is unchanged from before the house-UI migration; every value now
// mirrors a zqsfx::ui token (docs/ZQSFX_UI_STYLE_GUIDE.md / ../../../docs/ZQSFX_UI_STYLE_GUIDE.md),
// so this project keeps its own single colour source while reading as the same family as every
// other ZQ SFX product. See docs/ui_migration_report.md for the full mapping table.
namespace
{
    namespace colour = zqsfx::ui::colour;
    namespace comp = zqsfx::ui::comp;
}

const juce::Colour ModernLookAndFeel::Colors::background          = colour::chassisMid;
const juce::Colour ModernLookAndFeel::Colors::backgroundSecondary = colour::panelBot;
const juce::Colour ModernLookAndFeel::Colors::surface             = colour::panelTop;
const juce::Colour ModernLookAndFeel::Colors::surfaceHover        = colour::btnTop;

const juce::Colour ModernLookAndFeel::Colors::textPrimary         = colour::btnText;
const juce::Colour ModernLookAndFeel::Colors::textSecondary       = colour::silkLabel;
const juce::Colour ModernLookAndFeel::Colors::textDisabled        = colour::silkCaption;

const juce::Colour ModernLookAndFeel::Colors::primary             = colour::accent;
const juce::Colour ModernLookAndFeel::Colors::primaryHover        = colour::accent.brighter(0.15f);
const juce::Colour ModernLookAndFeel::Colors::primaryPressed      = colour::accentDim;
const juce::Colour ModernLookAndFeel::Colors::secondary           = comp::sky;

const juce::Colour ModernLookAndFeel::Colors::success             = colour::lcdText;
const juce::Colour ModernLookAndFeel::Colors::warning             = colour::meterHot; // 0xffD9A441
const juce::Colour ModernLookAndFeel::Colors::error               = colour::warn;
const juce::Colour ModernLookAndFeel::Colors::info                = comp::sky; // no dedicated house "info" token; not used by this project today

const juce::Colour ModernLookAndFeel::Colors::border              = colour::ruleInner;
const juce::Colour ModernLookAndFeel::Colors::borderHover         = colour::ruleTitle;
const juce::Colour ModernLookAndFeel::Colors::borderFocus         = colour::accent;
const juce::Colour ModernLookAndFeel::Colors::outline             = colour::accent;

//==============================================================================
ModernLookAndFeel::ModernLookAndFeel()
{
    // Everything the house zqsfx::ui::LookAndFeel constructor already sets (ComboBox/PopupMenu ->
    // LCD glass, Slider textbox -> LCD glass + glow, TextButton -> btn gradient / accent-on,
    // TooltipWindow/AlertWindow/TextEditor -> house tokens) needs no re-setting here.
    //
    // TabbedComponent::backgroundColourId/outlineColourId and the per-tab colour passed to
    // addTab() are drawn directly by TabbedComponent::paint() (not through a LookAndFeel virtual),
    // so they stay live and are set here; TabbedButtonBar's own tab*/front* colour ids are only
    // ever read by LookAndFeel_V4's default drawTabButton, which this class fully replaces below,
    // so they are not set (they would be dead).
    setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
    setColour(juce::TabbedComponent::backgroundColourId, Colors::background);
    setColour(juce::TabbedComponent::outlineColourId, Colors::border);

    // Read directly by our own drawToggleButton below.
    setColour(juce::ToggleButton::textColourId, Colors::textPrimary);
}

ModernLookAndFeel::~ModernLookAndFeel()
{
}

//==============================================================================
// Group components: the house LookAndFeel has no drawGroupComponentOutline equivalent, so this
// stays -- restyled to mirror zqsfx::ui::Panel's own paint() exactly (gradient face, hard 1px
// panelBorder, platform-bold wide-tracked silkTitle title, ruleTitle hairline). No rounded
// corners (style guide section 6). The group's own textColourId is intentionally not consulted,
// matching Panel (which has no per-instance title colour either); per-instance
// GroupComponent::textColourId calls at call sites were removed as dead code.
void ModernLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                                 const juce::String& text,
                                                 const juce::Justification& /*position*/,
                                                 juce::GroupComponent& /*group*/)
{
    namespace colour = zqsfx::ui::colour;

    auto r = juce::Rectangle<float>(0, 0, (float) width, (float) height);
    g.setGradientFill(zqsfx::ui::gradients::panel(r));
    g.fillRect(r);
    g.setColour(juce::Colours::white.withAlpha(0.04f)); // inner top highlight
    g.fillRect(r.withHeight(1.0f).translated(0.0f, 1.0f));
    g.setColour(colour::panelBorder);
    g.drawRect(r, 1.0f);

    if (text.isNotEmpty())
    {
        auto head = juce::Rectangle<int>(0, 0, width, height).reduced(8, 0).removeFromTop(20);
        g.setColour(colour::silkTitle);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)).withExtraKerningFactor(0.27f));
        g.drawText(text.toUpperCase(), head.translated(0, 4), juce::Justification::centredLeft);
        g.setColour(colour::ruleTitle);
        g.fillRect(juce::Rectangle<int>(8, 24, width - 16, 1));
    }
}

//==============================================================================
// Tabbed components: the house LookAndFeel has no tab-bar treatment. The front (selected) tab
// uses `accent` -- selected is exactly the "active/lit/focused/selected" meaning the style guide
// reserves the one house accent for. Off tabs use the `btn` gradient, hover turning the legend
// accent, exactly like a house button. Hard-edged rectangles throughout (no rounded corners).
void ModernLookAndFeel::drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                                     bool isMouseOver, bool /*isMouseDown*/)
{
    namespace colour = zqsfx::ui::colour;

    auto bounds = button.getLocalBounds().toFloat();
    const bool front = button.isFrontTab();
    const bool enabled = button.isEnabled();
    const float alphaMul = enabled ? 1.0f : zqsfx::ui::geom::dimAlpha;

    if (front)
    {
        g.setColour(colour::accent.withAlpha(alphaMul));
        g.fillRect(bounds);
        g.setColour(juce::Colours::black.withAlpha(0.35f * alphaMul));
        g.fillRect(bounds.withTop(bounds.getBottom() - 2.0f));
    }
    else
    {
        g.setGradientFill(zqsfx::ui::gradients::button(bounds, enabled));
        g.fillRect(bounds);
        g.setColour(juce::Colours::white.withAlpha(enabled ? 0.07f : 0.0f));
        g.fillRect(bounds.withHeight(1.0f));
    }
    g.setColour(colour::btnBorder.withAlpha(alphaMul));
    g.drawRect(button.getLocalBounds().toFloat(), 1.0f);

    const auto textColour = !enabled ? colour::silkCaption
                           : front    ? colour::accentInk
                           : isMouseOver ? colour::accent
                                         : colour::btnText;
    g.setColour(textColour);
    g.setFont(silkFont(front ? 14.0f : 13.0f, true));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(Spacing::sm, Spacing::xs),
                      juce::Justification::centred, 1);
}

int ModernLookAndFeel::getTabButtonBestWidth(juce::TabBarButton& button, int /*tabDepth*/)
{
    // The house silk face (Barlow Condensed) is wider than JUCE's default sans at the same point
    // size, so this measures the actual house font rather than assuming JUCE's default metrics.
    auto font = silkFont(14.0f, true);
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, button.getButtonText(), 0.0f, 0.0f);
    int textWidth = (int) std::ceil(glyphs.getBoundingBox(0, -1, false).getWidth());
    return textWidth + Spacing::lg * 2; // 24px padding each side
}

void ModernLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar& bar, juce::Graphics& g)
{
    namespace colour = zqsfx::ui::colour;

    auto bounds = bar.getLocalBounds();
    g.setColour(Colors::background);
    g.fillRect(bounds);
    g.setColour(colour::ruleTitle);
    g.fillRect(juce::Rectangle<int>(bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1));
}

//==============================================================================
// Progress bar: the house LookAndFeel has no progress-bar treatment. Drawn as a phosphor screen
// (the same bezel/glass every readout and meter uses) with an `accent` fill -- accent means
// "active", and a progress bar's fill is literally showing how far the active operation has got.
void ModernLookAndFeel::drawProgressBar(juce::Graphics& g, juce::ProgressBar&,
                                       int width, int height, double progress,
                                       const juce::String& textToShow)
{
    namespace colour = zqsfx::ui::colour;

    auto bounds = juce::Rectangle<float>(0, 0, (float) width, (float) height);
    drawScreen(g, bounds, height > 20);

    auto glass = bounds.reduced(2.0f);
    if (progress >= 0.0)
    {
        auto fillWidth = glass.getWidth() * (float) juce::jlimit(0.0, 1.0, progress);
        g.setColour(colour::accent.withAlpha(0.55f));
        g.fillRect(glass.withWidth(fillWidth));
    }

    if (textToShow.isNotEmpty())
        drawLcdText(g, textToShow, bounds.toNearestInt(), (float) height * 0.6f);
}

//==============================================================================
// Toggle buttons (checkboxes): the house's bound LitToggle/TextToggle are APVTS controls this
// project has no parameters for, so plain juce::ToggleButton keeps its own tick-box drawing --
// restyled with house tokens: hard-edged square (no rounded corners), `btn` gradient off-state,
// `accent` fill + `accentInk` check on-state, hover turns the legend `accent` (matching the
// house TextButton hover convention). Keyboard focus is left to the house's automatic focus
// outline; nothing here draws its own ring.
void ModernLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool /*shouldDrawButtonAsDown*/)
{
    namespace colour = zqsfx::ui::colour;

    auto bounds = button.getLocalBounds();
    constexpr float checkboxSize = 16.0f;
    auto checkboxBounds = juce::Rectangle<float>(0, 0, checkboxSize, checkboxSize)
                             .withCentre(juce::Point<float>(checkboxSize * 0.5f + (float) Spacing::xs,
                                                            (float) bounds.getCentreY()));

    const bool on = button.getToggleState();
    const bool enabled = button.isEnabled();
    const float alphaMul = enabled ? 1.0f : zqsfx::ui::geom::dimAlpha;

    if (on)
    {
        g.setColour(colour::accent.withAlpha(alphaMul));
        g.fillRect(checkboxBounds);
        g.setColour(juce::Colours::black.withAlpha(0.35f * alphaMul));
        g.fillRect(checkboxBounds.withTop(checkboxBounds.getBottom() - 2.0f));
    }
    else
    {
        g.setGradientFill(zqsfx::ui::gradients::button(checkboxBounds, enabled));
        g.fillRect(checkboxBounds);
        g.setColour(juce::Colours::white.withAlpha(enabled ? 0.07f : 0.0f));
        g.fillRect(checkboxBounds.withHeight(1.0f));
    }
    g.setColour(colour::btnBorder.withAlpha(alphaMul));
    g.drawRect(checkboxBounds, 1.0f);

    if (on)
    {
        g.setColour(colour::accentInk.withAlpha(alphaMul));
        juce::Path checkmark;
        checkmark.startNewSubPath(checkboxBounds.getX() + 3, checkboxBounds.getCentreY());
        checkmark.lineTo(checkboxBounds.getCentreX() - 1, checkboxBounds.getBottom() - 4);
        checkmark.lineTo(checkboxBounds.getRight() - 3, checkboxBounds.getY() + 4);
        g.strokePath(checkmark, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Text
    auto textBounds = bounds.withTrimmedLeft((int) (checkboxSize + Spacing::sm + Spacing::xs));
    if (!textBounds.isEmpty())
    {
        g.setFont(silkFont(13.0f, false));

        juce::Colour textColour;
        if (!enabled)
            textColour = colour::silkCaption;
        else if (shouldDrawButtonAsHighlighted && !on)
            textColour = colour::accent; // hover turns the legend accent, same as house buttons
        else
            textColour = button.findColour(juce::ToggleButton::textColourId);

        g.setColour(textColour);
        g.drawFittedText(button.getButtonText(), textBounds, juce::Justification::centredLeft, 1);
    }
}

//==============================================================================
void ModernLookAndFeel::drawDropZoneHint(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                        const juce::String& hintText)
{
    namespace colour = zqsfx::ui::colour;

    auto area = bounds.reduced(Spacing::xl);

    // Hard-edged rectangle, no rounded corners (style guide section 6).
    g.setColour(colour::ruleTitle);
    g.drawRect(area.toFloat(), 1.5f);

    g.setColour(colour::silkCaption);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText(hintText, area, juce::Justification::centred, true);
}
