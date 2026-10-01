#pragma once

#include <JuceHeader.h>
#include "../Utils/UCSManager.h"
#include "../Utils/ConfigManager.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
/**
 * UCS Naming Panel - UI for Universal Category System naming
 *
 * Provides professional sound effects naming according to UCS standards:
 * Category_Subcategory_Description_Channel.wav
 *
 * Features:
 * - Category dropdown with UCS taxonomy
 * - Subcategory dropdown (filtered by category)
 * - Description text field
 * - Live filename preview
 * - Auto-detection of existing UCS filenames
 * - Smart defaults and remembering last-used values
 */
class UCSNamingPanel : public juce::Component
{
public:
    //==============================================================================
    UCSNamingPanel(UCSManager* ucsManager, ConfigManager* config = nullptr);
    ~UCSNamingPanel() override;

    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    /** Height the content needs so the Preview row is not clipped. Computed from the same
        constants resized() uses; the CollapsiblePanel wrapping this component must request
        exactly this much (the page scrolls rather than squeezing it). */
    static int getPreferredContentHeight();

    /** True while the Batch tab is showing. UCS names are applied by the Single File tab only
        (one category and description over many files would give every file the same name), so
        the preview says so instead of promising a name the batch will not use. */
    void setBatchTabActive(bool batchActive);

    //==============================================================================
    // Settings access

    /**
     * Get selected category code (e.g., "AMBNat")
     */
    juce::String getCategory() const;

    /**
     * Get selected subcategory (e.g., "Forest")
     */
    juce::String getSubcategory() const;

    /**
     * Get description text (e.g., "MorningBirds")
     */
    juce::String getDescription() const;

    /**
     * Check if UCS naming is enabled
     */
    bool isUCSEnabled() const;

    /**
     * Set category by code
     */
    void setCategory(const juce::String& categoryCode);

    /**
     * Set subcategory by name
     */
    void setSubcategory(const juce::String& subcategory);

    /**
     * Set description
     */
    void setDescription(const juce::String& description);

    /**
     * Enable/disable UCS naming
     */
    void setUCSEnabled(bool enabled);

    /**
     * Parse and populate from existing filename
     * @param filename Filename to parse (can be UCS or regular format)
     */
    void parseFromFilename(const juce::String& filename);

    /**
     * Generate filename preview for given channel suffix
     * @param channelSuffix Channel suffix (e.g., "L", "R", "M")
     * @return Formatted UCS filename
     */
    juce::String generateFilename(const juce::String& channelSuffix) const;

    /**
     * Update filename preview label
     * Should be called when channel count changes externally
     */
    void updatePreview();

    //==============================================================================
    // Callbacks

    /**
     * Called when UCS settings change
     */
    std::function<void()> onSettingsChanged;

private:
    //==============================================================================
    // UI Components

    // Enable/disable UCS naming
    juce::ToggleButton enableUCSToggle;

    // Category selection
    juce::Label categoryLabel;
    juce::ComboBox categoryCombo;

    // Subcategory selection
    juce::Label subcategoryLabel;
    juce::ComboBox subcategoryCombo;

    // Description field
    juce::Label descriptionLabel;
    juce::TextEditor descriptionEditor;

    // Preview
    juce::Label previewLabel;
    juce::Label previewText;

    // Group box
    juce::GroupComponent ucsGroup;

    //==============================================================================
    // Layout constants shared by resized() and getPreferredContentHeight()
    static constexpr int kPadding = 8;
    static constexpr int kRowHeight = 28;
    static constexpr int kRowGap = 4;

    //==============================================================================
    // Data

    UCSManager* ucsManager {nullptr};
    ConfigManager* configManager {nullptr};

    // True when the UCS taxonomy failed to load (see showTaxonomyUnavailableState()). Naming
    // controls stay disabled and the preview shows why, instead of silently doing nothing.
    bool taxonomyUnavailable {false};

    // See setBatchTabActive().
    bool batchTabActive {false};

    //==============================================================================
    // Helper methods

    /**
     * Populate category dropdown from UCS taxonomy
     */
    void populateCategoryCombo();

    /**
     * Populate subcategory dropdown for selected category
     */
    void populateSubcategoryCombo();

    /**
     * Handle category selection change
     */
    void categoryChanged();

    /**
     * Handle any UCS setting change
     */
    void settingsChanged();

    /**
     * Update filename preview display
     */
    void updateFilenamePreview();

    /**
     * Enable/disable UCS controls based on toggle
     */
    void updateControlStates();

    /**
     * Make a failed taxonomy load visible and actionable: disables the naming controls (so the
     * user cannot interact with empty dropdowns) and shows a clear inline message explaining why,
     * instead of leaving the panel looking merely empty. Project rule: no silent failures.
     */
    void showTaxonomyUnavailableState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UCSNamingPanel)
};
