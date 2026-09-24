#include "OptionsPanel.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
OptionsPanel::OptionsPanel(ConfigManager* config)
    : configManager(config),
      sampleRateGroup("sampleRateGroup", "Sample Rate"),
      overrideSampleRateToggle("Override Sample Rate"),  // mirrors Python line 1579
      sampleRateLabel("sampleRateLabel", "Sample Rate:"),
      bitDepthGroup("bitDepthGroup", "Bit Depth"),
      overrideBitDepthToggle("Override Bit Depth"),      // mirrors Python line 1604
      bitDepthLabel("bitDepthLabel", "Bit Depth:"),
      customNamesGroup("customNamesGroup", "Custom Channel Names"),
      customNamesLabel("customNamesLabel", "Channel Names:"),
      customNamesHelpLabel("customNamesHelpLabel", "(comma-separated)"),
      optionsGroup("optionsGroup", "Options"),
      stereoToMonoToggle("Convert stereo to mono"),
      preserveIXMLToggle("Preserve iXML metadata"),
      channelRemappingToggle("Enable channel remapping")
{
    // Setup sample rate options - mirrors Python lines 1576-1592
    // (GroupComponent::textColourId, ComboBox::backgroundColourId/textColourId/outlineColourId
    // are no longer read: the house LookAndFeel draws every GroupComponent/ComboBox its own
    // fixed way -- see ModernLookAndFeel::drawGroupComponentOutline and
    // zqsfx::ui::LookAndFeel::drawComboBox/positionComboBoxText -- so those per-instance calls
    // were dead code and have been removed.)
    setupSampleRateOptions();
    addAndMakeVisible(sampleRateGroup);

    // Override checkbox - mirrors Python lines 1577-1589
    overrideSampleRateToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    overrideSampleRateToggle.setTooltip("Override the source file's sample rate in output files");
    overrideSampleRateToggle.setTitle("Override Sample Rate");
    overrideSampleRateToggle.setDescription("Override the source file's sample rate in output files");
    overrideSampleRateToggle.onClick = [this]() { toggleSampleRateDropdown(); };
    addAndMakeVisible(overrideSampleRateToggle);

    sampleRateLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(sampleRateLabel);

    sampleRateCombo.onChange = [this]() { settingsChanged(); };
    sampleRateCombo.setEnabled(false); // Initially disabled - mirrors Python line 87
    sampleRateCombo.setTooltip("Select target sample rate for output files");
    sampleRateCombo.setTitle("Sample Rate");
    sampleRateCombo.setDescription("Select target sample rate for output files");
    addAndMakeVisible(sampleRateCombo);

    // Setup bit depth options - mirrors Python lines 1594-1610
    setupBitDepthOptions();
    addAndMakeVisible(bitDepthGroup);

    // Override checkbox - mirrors Python lines 1602-1614
    overrideBitDepthToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    overrideBitDepthToggle.setTooltip("Override the source file's bit depth in output files");
    overrideBitDepthToggle.setTitle("Override Bit Depth");
    overrideBitDepthToggle.setDescription("Override the source file's bit depth in output files");
    overrideBitDepthToggle.onClick = [this]() { toggleBitDepthDropdown(); };
    addAndMakeVisible(overrideBitDepthToggle);

    bitDepthLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(bitDepthLabel);

    bitDepthCombo.onChange = [this]() { settingsChanged(); };
    bitDepthCombo.setEnabled(false); // Initially disabled - mirrors Python line 93
    bitDepthCombo.setTooltip("Select target bit depth for output files");
    bitDepthCombo.setTitle("Bit Depth");
    bitDepthCombo.setDescription("Select target bit depth for output files");
    addAndMakeVisible(bitDepthCombo);

    // Setup custom names - mirrors Python lines 1612-1628
    setupCustomNames();
    addAndMakeVisible(customNamesGroup);

    customNamesLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(customNamesLabel);

    customNamesHelpLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(customNamesHelpLabel);

    customNamesEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    customNamesEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    customNamesEditor.setTooltip("Enter custom channel names separated by commas (e.g., L,R,C,Lfe,Ls,Rs)");
    customNamesEditor.onTextChange = [this]() { settingsChanged(); };
    addAndMakeVisible(customNamesEditor);

    // Setup options toggles - mirrors Python lines 1630-1701
    setupOptionsToggles();
    addAndMakeVisible(optionsGroup);

    stereoToMonoToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    stereoToMonoToggle.setTooltip("Mix stereo pairs to mono output files");
    stereoToMonoToggle.setTitle("Convert Stereo to Mono");
    stereoToMonoToggle.setDescription("Mix stereo pairs to mono output files");
    stereoToMonoToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(stereoToMonoToggle);

    preserveIXMLToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    preserveIXMLToggle.setTooltip("Preserve iXML metadata in output files (required for Soundminer compatibility)");
    preserveIXMLToggle.setTitle("Preserve iXML Metadata");
    preserveIXMLToggle.setDescription("Preserve iXML metadata in output files (required for Soundminer compatibility)");
    preserveIXMLToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(preserveIXMLToggle);

    channelRemappingToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    channelRemappingToggle.setTooltip("Enable channel remapping for non-standard channel layouts");
    channelRemappingToggle.setTitle("Enable Channel Remapping");
    channelRemappingToggle.setDescription("Enable channel remapping for non-standard channel layouts");
    channelRemappingToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(channelRemappingToggle);
    
    juce::Logger::writeToLog("OptionsPanel initialized");
    juce::Logger::writeToLog("Sample rate combo items: " + juce::String(sampleRateCombo.getNumItems()));
    juce::Logger::writeToLog("Bit depth combo items: " + juce::String(bitDepthCombo.getNumItems()));
}

//==============================================================================
OptionsPanel::~OptionsPanel()
{
}

//==============================================================================
void OptionsPanel::paint(juce::Graphics& g)
{
    // Use modern background color
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
void OptionsPanel::resized()
{
    auto bounds = getLocalBounds();
    const int margin = ModernLookAndFeel::Spacing::md;           // 16px modern spacing
    const int spacing = ModernLookAndFeel::Spacing::sm;          // 8px modern spacing

    // Calculate responsive group height based on available space
    int groupHeight = juce::jmax(kMinGroupHeight, bounds.getHeight() / 4);  // Adjusted ratio for better fit

    bounds.reduce(margin, margin);

    // Top row (Sample Rate / Bit Depth / Options) takes groupHeight off the top; everything the
    // Custom Names group below lays out from what is LEFT of `bounds` after that -- never from
    // getLocalBounds() -- so the two rows can never overlap regardless of panel height.
    auto topRowArea = bounds.removeFromTop(groupHeight);
    bounds.removeFromTop(spacing);

    // Layout top row in columns to match Python grid layout
    auto leftColumn = topRowArea.removeFromLeft(topRowArea.getWidth() / 3);
    auto middleColumn = topRowArea.removeFromLeft(topRowArea.getWidth() / 2);
    auto rightColumn = topRowArea;

    // Sample rate group - left column
    auto sampleRateArea = leftColumn;
    sampleRateGroup.setBounds(sampleRateArea);
    // Start positioning from inside the group, accounting for the title
    auto sampleRateInner = sampleRateArea;
    sampleRateInner.removeFromTop(20); // Space for group title
    sampleRateInner = sampleRateInner.reduced(spacing, spacing);

    overrideSampleRateToggle.setBounds(sampleRateInner.removeFromTop(25));
    sampleRateInner.removeFromTop(5);  // Add spacing
    sampleRateLabel.setBounds(sampleRateInner.removeFromTop(20));
    sampleRateInner.removeFromTop(5);  // Add spacing
    sampleRateCombo.setBounds(sampleRateInner.removeFromTop(35));  // Taller combo

    // Bit depth group - middle column
    auto bitDepthArea = middleColumn;
    bitDepthGroup.setBounds(bitDepthArea);
    // Start positioning from inside the group, accounting for the title
    auto bitDepthInner = bitDepthArea;
    bitDepthInner.removeFromTop(20); // Space for group title
    bitDepthInner = bitDepthInner.reduced(spacing, spacing);

    overrideBitDepthToggle.setBounds(bitDepthInner.removeFromTop(25));
    bitDepthInner.removeFromTop(5);  // Add spacing
    bitDepthLabel.setBounds(bitDepthInner.removeFromTop(20));
    bitDepthInner.removeFromTop(5);  // Add spacing
    bitDepthCombo.setBounds(bitDepthInner.removeFromTop(35));  // Taller combo

    // Options group - right column
    auto optionsArea = rightColumn;
    optionsGroup.setBounds(optionsArea);
    auto optionsInner = optionsArea.reduced(spacing * 2, spacing * 2);  // Consistent padding
    stereoToMonoToggle.setBounds(optionsInner.removeFromTop(20));
    preserveIXMLToggle.setBounds(optionsInner.removeFromTop(20));
    channelRemappingToggle.setBounds(optionsInner);

    // Custom names group - spans full width, using whatever remains of `bounds` below the top
    // row (already margin-reduced left/right/bottom by the initial bounds.reduce() above).
    auto customNamesArea = bounds;
    customNamesGroup.setBounds(customNamesArea);
    auto customNamesInner = customNamesArea.reduced(spacing * 2, spacing * 2);  // Consistent padding
    customNamesInner.removeFromTop(20); // Space for group title, as the three groups above do

    auto customNamesRow = customNamesInner.removeFromTop(30);  // Larger row height
    customNamesLabel.setBounds(customNamesRow.removeFromLeft(150));  // Wider label
    customNamesHelpLabel.setBounds(customNamesRow.removeFromRight(150));  // Wider help
    customNamesEditor.setBounds(customNamesRow);
}

//==============================================================================
int OptionsPanel::getPreferredContentHeight()
{
    const int margin = ModernLookAndFeel::Spacing::md;   // 16px, top + bottom (bounds.reduce)
    const int spacing = ModernLookAndFeel::Spacing::sm;  // 8px gap between the two rows

    // Mirrors resized(): margin (top) + top-row groupHeight + spacing + Custom Names groupHeight
    // + margin (bottom), at the minimum group height. Larger panel heights simply give
    // `groupHeight` (computed in resized() from the available height) more room; this is the
    // floor below which the two rows cannot both fit at a readable size.
    return margin * 2 + kMinGroupHeight * 2 + spacing;
}

//==============================================================================
// Settings access - mirrors Python variable access
bool OptionsPanel::getOverrideSampleRate() const
{
    return overrideSampleRateToggle.getToggleState();
}

juce::String OptionsPanel::getSampleRate() const
{
    return sampleRateCombo.getText();
}

bool OptionsPanel::getOverrideBitDepth() const
{
    return overrideBitDepthToggle.getToggleState();
}

juce::String OptionsPanel::getBitDepth() const
{
    return bitDepthCombo.getText();
}

juce::String OptionsPanel::getCustomNames() const
{
    return customNamesEditor.getText();
}

bool OptionsPanel::getStereoToMono() const
{
    return stereoToMonoToggle.getToggleState();
}

bool OptionsPanel::getPreserveIXML() const
{
    return preserveIXMLToggle.getToggleState();
}

bool OptionsPanel::getChannelRemapping() const
{
    return channelRemappingToggle.getToggleState();
}

//==============================================================================
// Helper methods
void OptionsPanel::setupSampleRateOptions()
{
    // mirrors Python sample rate setup (lines 1578-1592)
    // Python has: ["11025 Hz", "22050 Hz", "44100 Hz", "48000 Hz", "96000 Hz"]
    sampleRateCombo.addItem("11025 Hz", 1);
    sampleRateCombo.addItem("22050 Hz", 2);
    sampleRateCombo.addItem("44100 Hz", 3);
    sampleRateCombo.addItem("48000 Hz", 4);
    sampleRateCombo.addItem("96000 Hz", 5);
    
    // Set default to 48000 Hz - mirrors Python line 1219
    sampleRateCombo.setSelectedId(4); // 48000 Hz default
}

void OptionsPanel::setupBitDepthOptions()
{
    // mirrors Python bit depth setup (lines 1596-1610)
    // Python has: ["8 bit", "16 bit", "24 bit", "32 bit"]
    bitDepthCombo.addItem("8 bit", 1);
    bitDepthCombo.addItem("16 bit", 2);
    bitDepthCombo.addItem("24 bit", 3);
    bitDepthCombo.addItem("32 bit", 4);
    
    // Set default to 16 bit - mirrors Python line 1221
    bitDepthCombo.setSelectedId(2); // 16 bit default
}

void OptionsPanel::setupCustomNames()
{
    // mirrors Python custom names setup (lines 1614-1628)
    if (configManager)
    {
        customNamesEditor.setText(configManager->getDefaultCustomNames(), juce::dontSendNotification);
    }
    else
    {
        customNamesEditor.setText("L,R,C,lfe,Ls,Rs,Lss,Rss", juce::dontSendNotification);
    }
}

void OptionsPanel::setupOptionsToggles()
{
    // mirrors Python checkbox setup (lines 1630-1701)
    
    // Set defaults (mirrors Python default values)
    stereoToMonoToggle.setToggleState(false, juce::dontSendNotification);  // line 1215
    preserveIXMLToggle.setToggleState(true, juce::dontSendNotification);   // line 1217  
    channelRemappingToggle.setToggleState(false, juce::dontSendNotification); // line 1225
}

void OptionsPanel::settingsChanged()
{
    // Update config with current values
    if (configManager)
    {
        configManager->setDefaultSampleRate(getSampleRate());
        configManager->setDefaultBitDepth(getBitDepth());
        configManager->setDefaultCustomNames(getCustomNames());
    }
    
    // Trigger callback for MainComponent to update button states
    if (onSettingsChanged)
    {
        onSettingsChanged();
    }
}

void OptionsPanel::toggleSampleRateDropdown()
{
    // mirrors Python toggle_sample_rate_dropdown (line 83-87)
    bool isEnabled = overrideSampleRateToggle.getToggleState();
    sampleRateCombo.setEnabled(isEnabled);
    settingsChanged();
}

void OptionsPanel::toggleBitDepthDropdown()
{
    // mirrors Python toggle_bit_depth_dropdown (line 89-93)
    bool isEnabled = overrideBitDepthToggle.getToggleState();
    bitDepthCombo.setEnabled(isEnabled);
    settingsChanged();
}