#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Modern LookAndFeel for ZQ SFX Audio Splitter
 * 
 * Implements contemporary design principles:
 * - Clean, minimal aesthetic with proper spacing
 * - Consistent color scheme with semantic meaning
 * - Enhanced accessibility features
 * - Responsive typography hierarchy
 * - Smooth animations and transitions
 */
class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    //==============================================================================
    ModernLookAndFeel();
    ~ModernLookAndFeel() override;

    //==============================================================================
    // Color Scheme - Modern, accessible color palette
    struct Colors
    {
        // Primary colors
        static const juce::Colour background;          // Deep charcoal
        static const juce::Colour backgroundSecondary; // Lighter charcoal
        static const juce::Colour surface;             // Card backgrounds
        static const juce::Colour surfaceHover;        // Hover states
        
        // Text colors
        static const juce::Colour textPrimary;         // High contrast text
        static const juce::Colour textSecondary;       // Secondary text
        static const juce::Colour textDisabled;        // Disabled text
        
        // Accent colors
        static const juce::Colour primary;             // Brand blue
        static const juce::Colour primaryHover;        // Hover blue
        static const juce::Colour primaryPressed;      // Pressed blue
        static const juce::Colour secondary;           // Secondary accent
        
        // Semantic colors
        static const juce::Colour success;             // Green for success
        static const juce::Colour warning;             // Orange for warnings
        static const juce::Colour error;               // Red for errors
        static const juce::Colour info;                // Blue for info
        
        // Border and outline colors
        static const juce::Colour border;              // Default borders
        static const juce::Colour borderHover;         // Hover borders
        static const juce::Colour borderFocus;         // Focus rings
        static const juce::Colour outline;             // Accessibility outlines
    };

    //==============================================================================
    // Typography - Improved font sizes for better legibility
    struct Typography
    {
        static float getHeadingSize() { return 20.0f; }     // Increased from 18.0f
        static float getSubheadingSize() { return 18.0f; }  // Increased from 16.0f  
        static float getBodySize() { return 16.0f; }        // Increased from 14.0f
        static float getSmallSize() { return 14.0f; }       // Increased from 12.0f
        static float getCaptionSize() { return 13.0f; }     // Increased from 11.0f
        
        static juce::Font getHeadingFont() { return juce::Font(juce::FontOptions(getHeadingSize()).withStyle("Bold")); }
        static juce::Font getSubheadingFont() { return juce::Font(juce::FontOptions(getSubheadingSize()).withStyle("Bold")); }
        static juce::Font getBodyFont() { return juce::Font(juce::FontOptions(getBodySize())); }
        static juce::Font getSmallFont() { return juce::Font(juce::FontOptions(getSmallSize())); }
        static juce::Font getCaptionFont() { return juce::Font(juce::FontOptions(getCaptionSize())); }
    };

    //==============================================================================
    // Spacing - Consistent spacing system
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
    // Component styling overrides
    
    // Buttons
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override;
                            
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted, 
                       bool shouldDrawButtonAsDown) override;

    // Text editors
    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;
                                
    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override;

    // Labels
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // Group components
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                 const juce::String& text,
                                 const juce::Justification& position,
                                 juce::GroupComponent& group) override;

    // Tabbed components
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                      bool isMouseOver, bool isMouseDown) override;
                      
    void drawTabbedButtonBarBackground(juce::TabbedButtonBar& bar, juce::Graphics& g) override;

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;

    // Progress bars
    void drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar,
                        int width, int height, double progress,
                        const juce::String& textToShow) override;

    // Toggle buttons (checkboxes)
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    // ComboBox
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
                      
    juce::Label* createComboBoxTextBox(juce::ComboBox& box) override;

    //==============================================================================
    // Accessibility enhancements
    bool areScrollbarButtonsVisible() override { return false; } // Modern flat scrollbars
    
    // Focus ring drawing
    void drawFocusRing(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                      int thickness = 2);

    // Drop zone empty-state indicator (shared by splitter tabs)
    static void drawDropZoneHint(juce::Graphics& g, const juce::Rectangle<int>& bounds,
                                 const juce::String& hintText);

private:
    //==============================================================================
    // Helper methods
    void drawRoundedRectangle(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                            float cornerRadius, const juce::Colour& fillColour,
                            const juce::Colour& borderColour = juce::Colour(),
                            float borderThickness = 0.0f);

    juce::Colour getButtonColour(juce::Button& button, bool isHighlighted, bool isDown);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernLookAndFeel)
};