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
    // (GroupComponent::textColourId and ComboBox background/text/outline colour ids are no
    // longer read -- see ModernLookAndFeel::drawGroupComponentOutline and
    // zqsfx::ui::LookAndFeel::drawComboBox/positionComboBoxText -- so the per-instance calls
    // below were dead code and have been removed.)
    addAndMakeVisible(ucsGroup);

    // Enable toggle
    enableUCSToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    enableUCSToggle.setToggleState(true, juce::dontSendNotification); // Enabled by default
    enableUCSToggle.setTooltip("Enable UCS (Universal Category System) naming format for output files");
    enableUCSToggle.setTitle("Enable UCS Naming");
    enableUCSToggle.setDescription("Enable UCS (Universal Category System) naming format for output files");
    enableUCSToggle.onClick = [this]() { updateControlStates(); settingsChanged(); };
    addAndMakeVisible(enableUCSToggle);

    // Category
    categoryLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(categoryLabel);

    categoryCombo.setTooltip("Select UCS category (e.g., AMBNat, DSnExt, FOLExt)");
    categoryCombo.setTitle("UCS Category");
    categoryCombo.setDescription("Select UCS category (e.g., AMBNat, DSnExt, FOLExt)");
    categoryCombo.onChange = [this]() { categoryChanged(); };
    addAndMakeVisible(categoryCombo);

    // Subcategory
    subcategoryLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(subcategoryLabel);

    subcategoryCombo.setTooltip("Select UCS subcategory");
    subcategoryCombo.setTitle("UCS Subcategory");
    subcategoryCombo.setDescription("Select UCS subcategory");
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
    descriptionEditor.setTooltip("Enter a description for the sound (e.g., MorningBirds, CarPass, Footsteps)");
    descriptionEditor.onTextChange = [this]() { settingsChanged(); };
    addAndMakeVisible(descriptionEditor);

    // Preview
    previewLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    previewLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    addAndMakeVisible(previewLabel);

    // A generated-filename readout is DATA, not an active/lit/focused/selected state, so it
    // gets the house's LCD-content colour (Colors::success == zqsfx::ui::colour::lcdText),
    // never Colors::primary/accent (accent means "active" only -- style guide section 2). Note:
    // the requested monospaced font is not actually drawn -- every juce::Label renders through
    // zqsfx::ui::LookAndFeel::getLabelFont(), which always substitutes the house silk (Barlow
    // Condensed) face regardless of a Label's own setFont() call (confirmed by reading
    // Label::paint()/LookAndFeel_V2::drawLabel in the vendored JUCE source; see also LFlOw's own
    // migration report, which hit and documented the same behaviour).
    previewText.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::success);
    addAndMakeVisible(previewText);

    // Populate UI
    if (ucsManager && ucsManager->isLoaded())
    {
        populateCategoryCombo();
    }
    else if (ucsManager)
    {
        // MainComponent already attempted this load before constructing this panel; retrying
        // here only helps a caller that constructs UCSNamingPanel with a not-yet-loaded manager
        // (the embedded data itself does not change between attempts).
        if (ucsManager->loadTaxonomyFromBinaryData())
        {
            populateCategoryCombo();
        }
        else
        {
            juce::Logger::writeToLog("UCSNamingPanel: Failed to load UCS taxonomy");
            showTaxonomyUnavailableState();
        }
    }
    else
    {
        showTaxonomyUnavailableState();
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

int UCSNamingPanel::getPreferredContentHeight()
{
    // Mirrors resized() exactly: outer margin, group title, inner spacing, toggle, three
    // 30 px rows with a spacing gap each, a double gap, then the 30 px preview row.
    const int margin  = ModernLookAndFeel::Spacing::md;
    const int spacing = ModernLookAndFeel::Spacing::sm;
    const int rowHeight = 30;
    return margin * 2 + 25 + spacing * 2 + 25 + spacing
         + (rowHeight + spacing) * 3 + spacing + rowHeight;
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
    if (taxonomyUnavailable)
    {
        previewText.setText("UCS naming unavailable: taxonomy data failed to load. Reinstall the application if this persists.",
                             juce::dontSendNotification);
        return;
    }

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

void UCSNamingPanel::showTaxonomyUnavailableState()
{
    taxonomyUnavailable = true;

    // Force the toggle off and disable it: there is no taxonomy to name against, so offering a
    // control that looks interactive but can never populate a category would just be a subtler
    // silent failure.
    enableUCSToggle.setToggleState(false, juce::dontSendNotification);
    enableUCSToggle.setEnabled(false);
    enableUCSToggle.setTooltip("UCS naming is unavailable because the embedded taxonomy data failed to load. Reinstall the application if this persists.");

    categoryCombo.setEnabled(false);
    subcategoryCombo.setEnabled(false);
    descriptionEditor.setEnabled(false);

    // Text conveys the state (not colour alone, per style guide section 6); the error colour
    // just reinforces it.
    previewText.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::error);
    updateFilenamePreview();
}
