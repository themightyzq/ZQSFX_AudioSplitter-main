#pragma once

#include <JuceHeader.h>
#include <zqsfx_ui/zqsfx_ui.h>

//==============================================================================
/**
 * ZQ SFX Audio Splitter LookAndFeel.
 *
 * THIN SUBCLASS of zqsfx::ui::LookAndFeel (the ZQ SFX house look, spec of record:
 * ../../../docs/ZQSFX_UI_STYLE_GUIDE.md, i.e. JUCE/docs/ZQSFX_UI_STYLE_GUIDE.md in the workspace).
 * The house LookAndFeel supplies rotary knobs, combo boxes (LCD dropdowns), popups, labels, and
 * gradient buttons automatically once its own draw overrides are left un-overridden here. This
 * class keeps only the overrides the house LookAndFeel has no equivalent for -- toggle buttons
 * (tick boxes), group-component section outlines, the tab bar, and the progress bar -- all
 * restyled with house tokens: hard-edged rectangles, no rounded corners, `btn` gradient off-state,
 * `accent` fill + `accentInk` text on-state (style guide section 6). Keyboard focus rings come
 * from the house LookAndFeel's createFocusOutlineForComponent; nothing here draws its own.
 *
 * `Colors` remains the single colour source for this project's own code (every name kept, per
 * house instructions), but every value is now a house token (see ModernLookAndFeel.cpp for the
 * mapping table). No `juce::Colours::` and no raw `0xff` literal appears outside this file.
 */
class ModernLookAndFeel : public zqsfx::ui::LookAndFeel
{
public:
    //==============================================================================
    ModernLookAndFeel();
    ~ModernLookAndFeel() override;

    //==============================================================================
    // Color Scheme - every name kept from the pre-migration palette; every value now mirrors a
    // house token (see the .cpp for exactly which one).
    struct Colors
    {
        // Primary colors
        static const juce::Colour background;          // == zqsfx::ui::colour::chassisMid
        static const juce::Colour backgroundSecondary; // == zqsfx::ui::colour::panelBot
        static const juce::Colour surface;              // == zqsfx::ui::colour::panelTop
        static const juce::Colour surfaceHover;         // == zqsfx::ui::colour::btnTop

        // Text colors
        static const juce::Colour textPrimary;          // == zqsfx::ui::colour::btnText
        static const juce::Colour textSecondary;        // == zqsfx::ui::colour::silkLabel
        static const juce::Colour textDisabled;         // == zqsfx::ui::colour::silkCaption

        // Accent colors
        static const juce::Colour primary;              // == zqsfx::ui::colour::accent
        static const juce::Colour primaryHover;         // == accent, brightened 0.15
        static const juce::Colour primaryPressed;       // == zqsfx::ui::colour::accentDim
        static const juce::Colour secondary;            // == zqsfx::ui::comp::sky

        // Semantic colors
        static const juce::Colour success;              // == zqsfx::ui::colour::lcdText
        static const juce::Colour warning;              // == zqsfx::ui::colour::meterHot (the warm-amber meter-hot token)
        static const juce::Colour error;                // == zqsfx::ui::colour::warn
        static const juce::Colour info;                 // == zqsfx::ui::comp::sky (no separate house "info"; shares the one extra channel this project uses)

        // Border and outline colors
        static const juce::Colour border;               // == zqsfx::ui::colour::ruleInner
        static const juce::Colour borderHover;           // == zqsfx::ui::colour::ruleTitle
        static const juce::Colour borderFocus;           // == zqsfx::ui::colour::accent
        static const juce::Colour outline;              // == zqsfx::ui::colour::accent
    };

    //==============================================================================
    // Spacing - Consistent spacing system (unchanged; layout, not colour or type)
    struct Spacing
    {
        static constexpr int xs = 4;   // 4px
        static constexpr int sm = 8;   // 8px
        static constexpr int md = 16;  // 16px
        static constexpr int lg = 24;  // 24px
        static constexpr int xl = 32;  // 32px
        static constexpr int xxl = 48; // 48px
    };

    //==============================================================================
    // Component styling overrides kept ONLY where the house LookAndFeel has no equivalent.

    // Group components: the house has no drawGroupComponentOutline; this mirrors zqsfx::ui::Panel's
    // own paint() (gradient face, hard 1px border, platform-bold wide-tracked title, ruleTitle
    // hairline) so every section reads as a house panel.
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                 const juce::String& text,
                                 const juce::Justification& position,
                                 juce::GroupComponent& group) override;

    // Tabbed components: the house has no tab-bar treatment of its own.
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                      bool isMouseOver, bool isMouseDown) override;

    void drawTabbedButtonBarBackground(juce::TabbedButtonBar& bar, juce::Graphics& g) override;

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;

    // Progress bar: the house has no progress-bar treatment; drawn as a phosphor screen with an
    // `accent` fill (accent = "active", and a progress bar's fill literally shows the active
    // operation's advancement).
    void drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar,
                        int width, int height, double progress,
                        const juce::String& textToShow) override;

    // Toggle buttons (checkboxes): the house's bound LitToggle/TextToggle are APVTS controls this
    // app has no parameters for, so plain juce::ToggleButton keeps its own tick-box drawing here,
    // restyled with house tokens.
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    //==============================================================================
    // Accessibility enhancements
    bool areScrollbarButtonsVisible() override { return false; } // Modern flat scrollbars

    // Drop zone empty-state indicator (shared by splitter tabs). Hard-edged rectangle now (style
    // guide section 6); no LookAndFeel draw-method equivalent to defer to.
    static void drawDropZoneHint(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                 const juce::String& hintText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernLookAndFeel)
};
