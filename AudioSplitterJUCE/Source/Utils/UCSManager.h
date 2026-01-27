#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * UCS Manager - Universal Category System taxonomy management
 *
 * Handles loading, parsing, and accessing the UCS taxonomy for professional
 * sound effects naming according to universalcategorysystem.com standards.
 *
 * Responsibilities:
 * - Load UCS taxonomy from JSON file
 * - Provide category and subcategory lists
 * - Generate UCS-compliant filenames
 * - Parse existing UCS filenames
 * - Validate UCS naming format
 * - Manage channel suffix conventions
 */
class UCSManager
{
public:
    //==============================================================================
    struct CategoryInfo
    {
        juce::String code;              // e.g., "AMBNat"
        juce::String name;              // e.g., "Natural Ambience"
        juce::String description;       // e.g., "Natural environmental sounds..."
        juce::StringArray subcategories; // e.g., ["Forest", "Ocean", "River"]
    };

    struct ParsedUCSFilename
    {
        bool isValid {false};
        juce::String category;
        juce::String subcategory;
        juce::String description;
        juce::String channelSuffix;
        juce::String extension;
    };

    //==============================================================================
    UCSManager();
    ~UCSManager();

    /**
     * Load UCS taxonomy from JSON file
     * @param taxonomyFilePath Path to ucs_taxonomy.json
     * @return true if loaded successfully
     */
    bool loadTaxonomy(const juce::File& taxonomyFile);

    /**
     * Load UCS taxonomy from embedded binary data
     * @return true if loaded successfully
     */
    bool loadTaxonomyFromBinaryData();

    /**
     * Check if taxonomy is loaded and ready
     */
    bool isLoaded() const { return taxonomyLoaded; }

    //==============================================================================
    // Category Access

    /**
     * Get list of all category codes (e.g., "AMBNat", "FOLExt")
     */
    juce::StringArray getCategoryCodes() const;

    /**
     * Get list of all category display names (e.g., "AMBNat - Natural Ambience")
     */
    juce::StringArray getCategoryDisplayNames() const;

    /**
     * Get category information by code
     */
    CategoryInfo getCategoryInfo(const juce::String& categoryCode) const;

    /**
     * Get subcategories for a specific category
     */
    juce::StringArray getSubcategories(const juce::String& categoryCode) const;

    //==============================================================================
    // Naming Generation

    /**
     * Generate UCS-compliant filename
     * @param category Category code (e.g., "AMBNat")
     * @param subcategory Subcategory name (e.g., "Forest")
     * @param description Custom description (e.g., "MorningBirds")
     * @param channelSuffix Channel suffix (e.g., "L", "R", "M")
     * @param extension File extension (default: ".wav")
     * @return Formatted filename: "Category_Subcategory_Description_Channel.wav"
     */
    juce::String generateFilename(const juce::String& category,
                                  const juce::String& subcategory,
                                  const juce::String& description,
                                  const juce::String& channelSuffix,
                                  const juce::String& extension = ".wav") const;

    /**
     * Sanitize description for filename safety
     * Removes illegal characters, enforces naming rules
     */
    juce::String sanitizeDescription(const juce::String& description) const;

    //==============================================================================
    // Parsing and Validation

    /**
     * Parse existing filename to extract UCS components
     * @param filename Filename to parse
     * @return Parsed components, isValid=false if not UCS format
     */
    ParsedUCSFilename parseFilename(const juce::String& filename) const;

    /**
     * Validate if filename follows UCS format
     */
    bool isValidUCSFilename(const juce::String& filename) const;

    /**
     * Validate if category code exists in taxonomy
     */
    bool isValidCategory(const juce::String& categoryCode) const;

    /**
     * Validate if subcategory exists for given category
     */
    bool isValidSubcategory(const juce::String& categoryCode, const juce::String& subcategory) const;

    //==============================================================================
    // Channel Suffix Management

    /**
     * Get standard channel suffix for channel index and count
     * @param channelIndex 0-based channel index
     * @param totalChannels Total number of channels
     * @return Channel suffix (e.g., "L", "R", "C", "Lfe")
     */
    juce::String getChannelSuffix(int channelIndex, int totalChannels) const;

    /**
     * Get all channel suffixes for a given channel count
     * @param channelCount Number of channels (1, 2, 6, 8, etc.)
     * @return Array of suffixes (e.g., ["L", "R", "C", "Lfe", "Ls", "Rs"])
     */
    juce::StringArray getChannelSuffixes(int channelCount) const;

private:
    //==============================================================================
    bool taxonomyLoaded {false};
    juce::var taxonomyData;  // Parsed JSON data

    // Category code -> CategoryInfo
    std::map<juce::String, CategoryInfo> categories;

    // Helper methods
    bool parseTaxonomyJSON(const juce::var& jsonData);
    juce::String cleanupWhitespace(const juce::String& input) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UCSManager)
};
