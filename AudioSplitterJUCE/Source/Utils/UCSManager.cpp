#include "UCSManager.h"
#include "AudioSplitterBinaryData.h"

//==============================================================================
UCSManager::UCSManager()
{
    juce::Logger::writeToLog("UCSManager initialized");
}

UCSManager::~UCSManager()
{
}

//==============================================================================
bool UCSManager::loadTaxonomy(const juce::File& taxonomyFile)
{
    if (!taxonomyFile.existsAsFile())
    {
        juce::Logger::writeToLog("UCSManager: Taxonomy file not found: " + taxonomyFile.getFullPathName());
        return false;
    }

    // Load and parse JSON
    juce::String jsonContent = taxonomyFile.loadFileAsString();
    juce::var parsedJSON;

    auto parseResult = juce::JSON::parse(jsonContent, parsedJSON);
    if (parseResult.failed())
    {
        juce::Logger::writeToLog("UCSManager: Failed to parse taxonomy JSON: " + parseResult.getErrorMessage());
        return false;
    }

    return parseTaxonomyJSON(parsedJSON);
}

bool UCSManager::loadTaxonomyFromBinaryData()
{
    // ucs_taxonomy.json is compiled into the binary via juce_add_binary_data (see
    // CMakeLists.txt / AudioSplitterBinaryData). UCS naming is this product's entire reason to
    // exist, so it must not depend on a "Resources" directory existing at some relative path
    // next to the installed executable -- that path assumption breaks the moment the app is
    // packaged into a .app bundle, notarized, or simply moved, and it used to fail silently.
    // Embedding removes the dependency entirely: there is no filesystem path left to break.
    juce::String jsonContent = juce::String::fromUTF8(AudioSplitterBinaryData::ucs_taxonomy_json,
                                                        AudioSplitterBinaryData::ucs_taxonomy_jsonSize);

    juce::var parsedJSON;
    auto parseResult = juce::JSON::parse(jsonContent, parsedJSON);
    if (parseResult.failed())
    {
        juce::Logger::writeToLog("UCSManager: Failed to parse embedded taxonomy JSON: " + parseResult.getErrorMessage());
        return false;
    }

    return parseTaxonomyJSON(parsedJSON);
}

bool UCSManager::parseTaxonomyJSON(const juce::var& jsonData)
{
    taxonomyData = jsonData;
    categories.clear();

    if (!taxonomyData.isObject())
    {
        juce::Logger::writeToLog("UCSManager: Invalid taxonomy data - not an object");
        return false;
    }

    // Parse categories
    auto* categoriesObject = taxonomyData.getProperty("categories", juce::var()).getDynamicObject();
    if (categoriesObject == nullptr)
    {
        juce::Logger::writeToLog("UCSManager: No categories found in taxonomy");
        return false;
    }

    // Iterate through all category codes
    for (auto& property : categoriesObject->getProperties())
    {
        juce::String categoryCode = property.name.toString();
        auto categoryData = property.value;

        CategoryInfo info;
        info.code = categoryCode;
        info.name = categoryData.getProperty("name", "").toString();
        info.description = categoryData.getProperty("description", "").toString();

        // Parse subcategories array
        if (categoryData.hasProperty("subcategories"))
        {
            auto* subcategoriesArray = categoryData.getProperty("subcategories", juce::var()).getArray();
            if (subcategoriesArray != nullptr)
            {
                for (auto& subcat : *subcategoriesArray)
                {
                    info.subcategories.add(subcat.toString());
                }
            }
        }

        categories[categoryCode] = info;
    }

    taxonomyLoaded = true;
    juce::Logger::writeToLog("UCSManager: Loaded " + juce::String(categories.size()) + " categories");

    return true;
}

//==============================================================================
juce::StringArray UCSManager::getCategoryCodes() const
{
    juce::StringArray codes;
    for (const auto& pair : categories)
    {
        codes.add(pair.first);
    }
    return codes;
}

juce::StringArray UCSManager::getCategoryDisplayNames() const
{
    juce::StringArray names;
    for (const auto& pair : categories)
    {
        // Format: "AMBNat - Natural Ambience"
        names.add(pair.first + " - " + pair.second.name);
    }
    return names;
}

UCSManager::CategoryInfo UCSManager::getCategoryInfo(const juce::String& categoryCode) const
{
    auto it = categories.find(categoryCode);
    if (it != categories.end())
    {
        return it->second;
    }
    return CategoryInfo(); // Return empty if not found
}

juce::StringArray UCSManager::getSubcategories(const juce::String& categoryCode) const
{
    auto it = categories.find(categoryCode);
    if (it != categories.end())
    {
        return it->second.subcategories;
    }
    return juce::StringArray();
}

//==============================================================================
juce::String UCSManager::generateFilename(const juce::String& category,
                                          const juce::String& subcategory,
                                          const juce::String& description,
                                          const juce::String& channelSuffix,
                                          const juce::String& extension) const
{
    // Sanitize inputs
    juce::String cleanCategory = category.trim();
    juce::String cleanSubcategory = subcategory.trim().removeCharacters(" ");
    juce::String cleanDescription = sanitizeDescription(description);
    juce::String cleanChannelSuffix = channelSuffix.trim();

    // Build filename: Category_Subcategory_Description_Channel.ext
    juce::String filename = cleanCategory;

    if (cleanSubcategory.isNotEmpty())
        filename += "_" + cleanSubcategory;

    if (cleanDescription.isNotEmpty())
        filename += "_" + cleanDescription;

    if (cleanChannelSuffix.isNotEmpty())
        filename += "_" + cleanChannelSuffix;

    // Add extension (ensure it starts with .)
    juce::String ext = extension;
    if (!ext.startsWithChar('.'))
        ext = "." + ext;

    filename += ext;

    return filename;
}

juce::String UCSManager::sanitizeDescription(const juce::String& description) const
{
    if (description.isEmpty())
        return description;

    juce::String cleaned = description.trim();

    // Remove illegal filename characters and characters problematic for metadata tools
    cleaned = cleaned.removeCharacters("\\/:*?\"<>|&%#$@!");

    // Replace spaces with underscores or remove them
    cleaned = cleaned.replace(" ", "_");

    // Remove consecutive underscores
    while (cleaned.contains("__"))
        cleaned = cleaned.replace("__", "_");

    // Remove leading/trailing underscores
    cleaned = cleaned.trimCharactersAtStart("_").trimCharactersAtEnd("_");

    return cleaned;
}

//==============================================================================
UCSManager::ParsedUCSFilename UCSManager::parseFilename(const juce::String& filename) const
{
    ParsedUCSFilename result;

    // Remove extension
    juce::String nameWithoutExt = filename.upToLastOccurrenceOf(".", false, false);
    result.extension = filename.fromLastOccurrenceOf(".", true, false);

    // Split by underscore
    juce::StringArray parts = juce::StringArray::fromTokens(nameWithoutExt, "_", "");

    // UCS format requires at least Category_Description_Channel
    // Minimum 3 parts: Category_Subcategory_Channel or Category_Description_Channel
    if (parts.size() < 3)
    {
        result.isValid = false;
        return result;
    }

    // First part is category
    result.category = parts[0];

    // Check if category is valid
    if (!isValidCategory(result.category))
    {
        result.isValid = false;
        return result;
    }

    // Last part is channel suffix
    result.channelSuffix = parts[parts.size() - 1];

    // Middle parts: could be Subcategory + Description, or just Description
    if (parts.size() == 3)
    {
        // Category_Description_Channel (no subcategory)
        result.description = parts[1];
    }
    else if (parts.size() >= 4)
    {
        // Category_Subcategory_Description_Channel
        // or Category_Subcategory_MultiWord_Description_Channel
        result.subcategory = parts[1];

        // Everything between subcategory and channel is description
        juce::StringArray descParts;
        for (int i = 2; i < parts.size() - 1; ++i)
        {
            descParts.add(parts[i]);
        }
        result.description = descParts.joinIntoString("_");
    }

    result.isValid = true;
    return result;
}

bool UCSManager::isValidUCSFilename(const juce::String& filename) const
{
    ParsedUCSFilename parsed = parseFilename(filename);
    return parsed.isValid;
}

bool UCSManager::isValidCategory(const juce::String& categoryCode) const
{
    return categories.find(categoryCode) != categories.end();
}

bool UCSManager::isValidSubcategory(const juce::String& categoryCode, const juce::String& subcategory) const
{
    auto it = categories.find(categoryCode);
    if (it == categories.end())
        return false;

    return it->second.subcategories.contains(subcategory);
}

//==============================================================================
juce::String UCSManager::getChannelSuffix(int channelIndex, int totalChannels) const
{
    juce::StringArray suffixes = getChannelSuffixes(totalChannels);

    if (channelIndex >= 0 && channelIndex < suffixes.size())
        return suffixes[channelIndex];

    // Fallback: return generic channel number
    return "Ch" + juce::String(channelIndex + 1);
}

juce::StringArray UCSManager::getChannelSuffixes(int channelCount) const
{
    juce::StringArray suffixes;

    // Standard UCS channel suffixes based on common channel layouts
    switch (channelCount)
    {
        case 1:
            suffixes.add("M"); // Mono
            break;

        case 2:
            suffixes.add("L");
            suffixes.add("R");
            break;

        case 3: // LCR
            suffixes.add("L");
            suffixes.add("C");
            suffixes.add("R");
            break;

        case 4: // Quad
            suffixes.add("L");
            suffixes.add("R");
            suffixes.add("Ls");
            suffixes.add("Rs");
            break;

        case 5: // 5.0
            suffixes.add("L");
            suffixes.add("R");
            suffixes.add("C");
            suffixes.add("Ls");
            suffixes.add("Rs");
            break;

        case 6: // 5.1
            suffixes.add("L");
            suffixes.add("R");
            suffixes.add("C");
            suffixes.add("Lfe");
            suffixes.add("Ls");
            suffixes.add("Rs");
            break;

        case 7: // 7.0
            suffixes.add("L");
            suffixes.add("R");
            suffixes.add("C");
            suffixes.add("Ls");
            suffixes.add("Rs");
            suffixes.add("Lss");
            suffixes.add("Rss");
            break;

        case 8: // 7.1
            suffixes.add("L");
            suffixes.add("R");
            suffixes.add("C");
            suffixes.add("Lfe");
            suffixes.add("Ls");
            suffixes.add("Rs");
            suffixes.add("Lss");
            suffixes.add("Rss");
            break;

        default:
            // For non-standard channel counts, use generic naming
            for (int i = 0; i < channelCount; ++i)
            {
                suffixes.add("Ch" + juce::String(i + 1));
            }
            break;
    }

    return suffixes;
}

//==============================================================================
juce::String UCSManager::cleanupWhitespace(const juce::String& input) const
{
    return input.trim().removeCharacters(" \t\n\r");
}
