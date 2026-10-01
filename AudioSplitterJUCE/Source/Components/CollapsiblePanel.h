#pragma once

#include <JuceHeader.h>
#include "../Utils/ConfigManager.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
/**
 * Collapsible Panel Component - wraps any content with expand/collapse header
 *
 * Features:
 * - Header bar with title and expand/collapse button
 * - Smooth animation between collapsed and expanded states
 * - Remembers state via ConfigManager
 * - Modern styling consistent with ModernLookAndFeel
 *
 * Usage:
 *   auto panel = std::make_unique<CollapsiblePanel>("Options", optionsPanel.get(), configManager, "options_collapsed");
 *   addAndMakeVisible(*panel);
 */
class CollapsiblePanel : public juce::Component
{
public:
    //==============================================================================
    /**
     * Create a collapsible panel
     * @param title Title to display in header
     * @param content Content component to wrap (panel takes ownership)
     * @param config ConfigManager for state persistence (can be nullptr)
     * @param stateKey Key for saving collapsed state (e.g., "options_collapsed")
     * @param startCollapsed Initial collapsed state (overridden by saved state if available)
     */
    CollapsiblePanel(const juce::String& title,
                    juce::Component* content,
                    ConfigManager* config = nullptr,
                    const juce::String& stateKey = juce::String(),
                    bool startCollapsed = false);

    ~CollapsiblePanel() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    //==============================================================================
    // State management

    /**
     * Check if panel is currently collapsed
     */
    bool isCollapsed() const { return collapsed; }

    /**
     * Set collapsed state (with animation)
     * @param shouldBeCollapsed New collapsed state
     * @param animate Whether to animate the transition
     */
    void setCollapsed(bool shouldBeCollapsed, bool animate = true);

    /**
     * Toggle between collapsed and expanded (with animation)
     */
    void toggleCollapsed();

    /**
     * Get the height this panel needs when expanded
     */
    int getExpandedHeight() const;

    /**
     * Get the height this panel needs when collapsed (header only)
     */
    int getCollapsedHeight() const;

    /**
     * Get current ideal height based on collapsed state
     */
    int getIdealHeight() const;

    /**
     * Set the content height explicitly (overrides auto-detection)
     */
    void setContentHeight(int height);

    //==============================================================================
    // Callbacks

    /**
     * Called when collapsed state changes
     * Useful for parent to adjust layout
     */
    std::function<void(bool isCollapsed)> onCollapseChanged;

private:
    //==============================================================================
    // Constants
    static constexpr int HEADER_HEIGHT = 32;
    static constexpr int MIN_CONTENT_HEIGHT = 100;

    //==============================================================================
    // UI Components

    // The header title is drawn directly in paint() (platform-bold, wide-tracked, silkTitle --
    // matching zqsfx::ui::Panel's own title exactly) rather than through a juce::Label, since a
    // Label's text always renders through the LookAndFeel's silk (Barlow Condensed) face.
    juce::String panelTitle;
    juce::DrawableButton collapseButton;
    std::unique_ptr<juce::Drawable> expandedIcon;
    std::unique_ptr<juce::Drawable> collapsedIcon;

    //==============================================================================
    // Content (owned by this panel)
    std::unique_ptr<juce::Component> contentComponent;
    int contentHeight {MIN_CONTENT_HEIGHT};

    //==============================================================================
    // State
    bool collapsed {false};
    ConfigManager* configManager {nullptr};
    juce::String stateKey;

    //==============================================================================
    // Animation
    juce::ComponentAnimator animator;

    //==============================================================================
    // Helper methods
    void createIcons();
    void updateCollapseButton();
    void saveState();
    void loadState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsiblePanel)
};
