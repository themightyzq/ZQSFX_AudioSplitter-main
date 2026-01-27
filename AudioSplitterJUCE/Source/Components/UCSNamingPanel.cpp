#include "UCSNamingPanel.h"

//==============================================================================
UCSNamingPanel::UCSNamingPanel(UCSManager* manager, ConfigManager* config)
    : ucsManager(manager),
      configManager(config),
      ucsGroup("ucsGroup", "UCS Naming (Universal Category System)"),
      enableUCSToggle("Enable UCS Naming"),
      categoryLabel("categoryLabel", "Category:"),
      subcategoryLabel("subcategoryLabel", "Subcategory:"),
      descriptionLabel("descriptionLabel", "Description:"),
      previewLabel("previewLabel", "Preview:"),
      previewText("previewText", "")
{
    // Setup UCS group
    ucsGroup.setColour(juce::GroupComponent::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(ucsGroup);

    // Enable toggle
    enableUCSToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    enableUCSToggle.setToggleState(true, juce::dontSendNotification); // Enabled by default
    enableUCSToggle.onClick = [this]() { updateControlStates(); settingsChanged(); };
    addAndMakeVisible(enableUCSToggle);

    // Category
    categoryLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(categoryLabel);

    categoryCombo.setColour(juce::ComboBox::backgroundColourId, ModernLookAndFeel::Colors::surface);
    categoryCombo.setColour(juce::ComboBox::textColourId, ModernLookAndFeel::Colors::textPrimary);
    categoryCombo.setColour(juce::ComboBox::outlineColourId, ModernLookAndFeel::Colors::border);
    categoryCombo.onChange = [this]() { categoryChanged(); };
    addAndMakeVisible(categoryCombo);

    // Subcategory
    subcategoryLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(subcategoryLabel);

    subcategoryCombo.setColour(juce::ComboBox::backgroundColourId, ModernLookAndFeel::Colors::surface);
    subcategoryCombo.setColour(juce::ComboBox::textColourId, ModernLookAndFeel::Colors::textPrimary);
    subcategoryCombo.setColour(juce::ComboBox::outlineColourId, ModernLookAndFeel::Colors::border);
    subcategoryCombo.onChange = [this]() { settingsChanged(); };
    addAndMakeVisible(subcategoryCombo);

    // Description
    descriptionLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(descriptionLabel);

    descriptionEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    descriptionEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    descriptionEditor.setColour(juce::TextEditor::outlineColourId, ModernLookAndFeel::Colors::border);
    descriptionEditor.setMultiLine(false);
    descriptionEditor.setReturnKeyStartsNewLine(false);
    descriptionEditor.setPopupMenuEnabled(true);
    descriptionEditor.onTextChange = [this]() { settingsChanged(); };
    addAndMakeVisible(descriptionEditor);

    // Preview
    previewLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    previewLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    addAndMakeVisible(previewLabel);

    previewText.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::primary);
    previewText.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain)));
    addAndMakeVisible(previewText);

    // Populate UI
    if (ucsManager && ucsManager->isLoaded())
    {
        populateCategoryCombo();
    }
    else if (ucsManager)
    {
        // Try loading taxonomy
        if (ucsManager->loadTaxonomyFromBinaryData())
        {
            populateCategoryCombo();
        }
        else
        {
            juce::Logger::writeToLog("UCSNamingPanel: Failed to load UCS taxonomy");
        }
    }

    updateControlStates();
    updateFilenamePreview();

    juce::Logger::writeToLog("UCSNamingPanel initialized");
}

UCSNamingPanel::~UCSNamingPanel()
{
}

//==============================================================================
void UCSNamingPanel::paint(juce::Graphics& g)
{
    g.fillAll(ModernLookAndFeel::Colors::background);
}

void UCSNamingPanel::resized()
{
    auto bounds = getLocalBounds();
    const int margin = ModernLookAndFeel::Spacing::md;
    const int spacing = ModernLookAndFeel::Spacing::sm;

    bounds.reduce(margin, margin);

    // Group encompasses entire panel
    ucsGroup.setBounds(getLocalBounds());

    // Start from inside the group
    bounds.removeFromTop(25); // Space for group title
    bounds.reduce(spacing, spacing);

    // Enable toggle at top
    auto toggleArea = bounds.removeFromTop(25);
    enableUCSToggle.setBounds(toggleArea);
    bounds.removeFromTop(spacing);

    // Three rows: Category, Subcategory, Description
    auto rowHeight = 30;
    auto labelWidth = 100;

    // Category row
    auto categoryRow = bounds.removeFromTop(rowHeight);
    categoryLabel.setBounds(categoryRow.removeFromLeft(labelWidth));
    categoryCombo.setBounds(categoryRow);
    bounds.removeFromTop(spacing);

    // Subcategory row
    auto subcategoryRow = bounds.removeFromTop(rowHeight);
    subcategoryLabel.setBounds(subcategoryRow.removeFromLeft(labelWidth));
    subcategoryCombo.setBounds(subcategoryRow);
    bounds.removeFromTop(spacing);

    // Description row
    auto descriptionRow = bounds.removeFromTop(rowHeight);
    descriptionLabel.setBounds(descriptionRow.removeFromLeft(labelWidth));
    descriptionEditor.setBounds(descriptionRow);
    bounds.removeFromTop(spacing * 2);

    // Preview row
    auto previewRow = bounds.removeFromTop(30);
    previewLabel.setBounds(previewRow.removeFromLeft(labelWidth));
    previewText.setBounds(previewRow);
}

//==============================================================================
juce::String UCSNamingPanel::getCategory() const
{
    if (!enableUCSToggle.getToggleState())
        return juce::String();

    int selectedId = categoryCombo.getSelectedId();
    if (selectedId <= 0)
        return juce::String();

    // Get category code from combo box text (format: "AMBNat - Natural Ambience")
    juce::String fullText = categoryCombo.getText();
    return fullText.upToFirstOccurrenceOf(" - ", false, false);
}

juce::String UCSNamingPanel::getSubcategory() const
{
    if (!enableUCSToggle.getToggleState())
        return juce::String();

    return subcategoryCombo.getText();
}

juce::String UCSNamingPanel::getDescription() const
{
    if (!enableUCSToggle.getToggleState())
        return juce::String();

    return descriptionEditor.getText();
}

bool UCSNamingPanel::isUCSEnabled() const
{
    return enableUCSToggle.getToggleState();
}

void UCSNamingPanel::setCategory(const juce::String& categoryCode)
{
    // Find the item with this category code
    for (int i = 1; i <= categoryCombo.getNumItems(); ++i)
    {
        juce::String itemText = categoryCombo.getItemText(i - 1);
        if (itemText.startsWith(categoryCode + " - "))
        {
            categoryCombo.setSelectedId(i, juce::sendNotification);
            return;
        }
    }
}

void UCSNamingPanel::setSubcategory(const juce::String& subcategory)
{
    // Find the item with this subcategory
    for (int i = 1; i <= subcategoryCombo.getNumItems(); ++i)
    {
        if (subcategoryCombo.getItemText(i - 1) == subcategory)
        {
            subcategoryCombo.setSelectedId(i, juce::sendNotification);
            return;
        }
    }
}

void UCSNamingPanel::setDescription(const juce::String& description)
{
    descriptionEditor.setText(description, juce::sendNotification);
}

void UCSNamingPanel::setUCSEnabled(bool enabled)
{
    enableUCSToggle.setToggleState(enabled, juce::sendNotification);
}

void UCSNamingPanel::parseFromFilename(const juce::String& filename)
{
    if (!ucsManager)
        return;

    // Try parsing as UCS filename
    auto parsed = ucsManager->parseFilename(filename);

    if (parsed.isValid)
    {
        // Populate UI from parsed components
        setCategory(parsed.category);
        setSubcategory(parsed.subcategory);
        setDescription(parsed.description);
        setUCSEnabled(true);

        juce::Logger::writeToLog("UCSNamingPanel: Parsed UCS filename - " +
                                 parsed.category + "_" + parsed.subcategory + "_" + parsed.description);
    }
    else
    {
        // Not a UCS filename - extract description from filename
        juce::String nameWithoutExt = filename.upToLastOccurrenceOf(".", false, false);
        setDescription(nameWithoutExt);
    }
}

juce::String UCSNamingPanel::generateFilename(const juce::String& channelSuffix) const
{
    if (!isUCSEnabled() || !ucsManager)
        return juce::String();

    return ucsManager->generateFilename(getCategory(),
                                       getSubcategory(),
                                       getDescription(),
                                       channelSuffix,
                                       ".wav");
}

void UCSNamingPanel::updatePreview()
{
    updateFilenamePreview();
}

//==============================================================================
void UCSNamingPanel::populateCategoryCombo()
{
    if (!ucsManager)
        return;

    categoryCombo.clear();

    auto displayNames = ucsManager->getCategoryDisplayNames();
    int id = 1;
    for (const auto& name : displayNames)
    {
        categoryCombo.addItem(name, id++);
    }

    // Select first item by default
    if (categoryCombo.getNumItems() > 0)
    {
        categoryCombo.setSelectedId(1, juce::sendNotification);
    }
}

void UCSNamingPanel::populateSubcategoryCombo()
{
    if (!ucsManager)
        return;

    subcategoryCombo.clear();

    juce::String categoryCode = getCategory();
    if (categoryCode.isEmpty())
        return;

    auto subcategories = ucsManager->getSubcategories(categoryCode);
    int id = 1;
    for (const auto& subcat : subcategories)
    {
        subcategoryCombo.addItem(subcat, id++);
    }

    // Select first item by default
    if (subcategoryCombo.getNumItems() > 0)
    {
        subcategoryCombo.setSelectedId(1, juce::sendNotification);
    }
}

void UCSNamingPanel::categoryChanged()
{
    // Repopulate subcategories when category changes
    populateSubcategoryCombo();
    settingsChanged();
}

void UCSNamingPanel::settingsChanged()
{
    // Update preview
    updateFilenamePreview();

    // Notify external listeners
    if (onSettingsChanged)
    {
        onSettingsChanged();
    }
}

void UCSNamingPanel::updateFilenamePreview()
{
    if (!isUCSEnabled())
    {
        previewText.setText("UCS naming disabled", juce::dontSendNotification);
        return;
    }

    // Generate preview with example channel suffix
    juce::String preview = generateFilename("L");

    if (preview.isEmpty())
    {
        previewText.setText("Select category and description", juce::dontSendNotification);
    }
    else
    {
        previewText.setText(preview, juce::dontSendNotification);
    }
}

void UCSNamingPanel::updateControlStates()
{
    bool enabled = enableUCSToggle.getToggleState();

    categoryCombo.setEnabled(enabled);
    subcategoryCombo.setEnabled(enabled);
    descriptionEditor.setEnabled(enabled);

    updateFilenamePreview();
}
