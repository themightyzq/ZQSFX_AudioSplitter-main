#include "OptionsPanel.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
OptionsPanel::OptionsPanel(ConfigManager* config)
    : configManager(config),
      sampleRateGroup("sampleRateGroup", "Sample Rate"),
      overrideSampleRateToggle("Override Sample Rate"),  // mirrors Python line 1579
      bitDepthGroup("bitDepthGroup", "Bit Depth"),
      overrideBitDepthToggle("Override Bit Depth"),      // mirrors Python line 1604
      customNamesGroup("customNamesGroup", "Custom Channel Names"),
      customNamesLabel("customNamesLabel", "Channel Names:"),
      customNamesHelpLabel("customNamesHelpLabel", "(comma-separated)"),
      optionsGroup("optionsGroup", "Options"),
      stereoToMonoToggle("Convert stereo to mono"),
      preserveIXMLToggle("Preserve iXML metadata")
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
    // Matches what AudioFileProcessor does: only a 2-channel file with both channels selected.
    const char* stereoToMonoTip = "For a 2-channel file with both channels selected, write one mono mix of L and R "
                                  "instead of two separate files. Files with other channel counts are split as usual.";
    stereoToMonoToggle.setTooltip(stereoToMonoTip);
    stereoToMonoToggle.setTitle("Convert Stereo to Mono");
    stereoToMonoToggle.setDescription(stereoToMonoTip);
    stereoToMonoToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(stereoToMonoToggle);

    preserveIXMLToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    preserveIXMLToggle.setTooltip("Preserve iXML metadata in output files (required for Soundminer compatibility)");
    preserveIXMLToggle.setTitle("Preserve iXML Metadata");
    preserveIXMLToggle.setDescription("Preserve iXML metadata in output files (required for Soundminer compatibility)");
    preserveIXMLToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(preserveIXMLToggle);

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
    constexpr int titleBand = ModernLookAndFeel::Spacing::groupTitleBand;
    constexpr int gap = ModernLookAndFeel::Spacing::sm;

    auto bounds = getLocalBounds().reduced(kPadding);

    // Every control sits below its group's title band (the OPTIONS title used to run into the
    // first checkbox because that group skipped this offset).
    auto insideGroup = [&](juce::GroupComponent& group, juce::Rectangle<int> area)
    {
        group.setBounds(area);
        return area.withTrimmedTop(titleBand).reduced(kPadding, 0).withTrimmedBottom(kPadding);
    };

    // Top row: three equal groups, fixed height. A short panel is never squeezed: the page
    // scrolls instead (MainComponent), so the layout never depends on available height.
    auto topRow = bounds.removeFromTop(kTopRowHeight);
    const int columnWidth = (topRow.getWidth() - 2 * gap) / 3;
    auto sampleRateArea = topRow.removeFromLeft(columnWidth);
    topRow.removeFromLeft(gap);
    auto bitDepthArea = topRow.removeFromLeft(columnWidth);
    topRow.removeFromLeft(gap);
    auto optionsArea = topRow;

    auto sampleRateInner = insideGroup(sampleRateGroup, sampleRateArea);
    overrideSampleRateToggle.setBounds(sampleRateInner.removeFromTop(kControlRowHeight));
    sampleRateInner.removeFromTop(4);
    sampleRateCombo.setBounds(sampleRateInner.removeFromTop(kControlRowHeight));

    auto bitDepthInner = insideGroup(bitDepthGroup, bitDepthArea);
    overrideBitDepthToggle.setBounds(bitDepthInner.removeFromTop(kControlRowHeight));
    bitDepthInner.removeFromTop(4);
    bitDepthCombo.setBounds(bitDepthInner.removeFromTop(kControlRowHeight));

    auto optionsInner = insideGroup(optionsGroup, optionsArea);
    stereoToMonoToggle.setBounds(optionsInner.removeFromTop(kControlRowHeight));
    optionsInner.removeFromTop(4);
    preserveIXMLToggle.setBounds(optionsInner.removeFromTop(kControlRowHeight));

    // Custom names: full width, one row, directly under the top row.
    bounds.removeFromTop(gap);
    auto namesInner = insideGroup(customNamesGroup, bounds.removeFromTop(kNamesRowHeight));
    auto namesRow = namesInner.removeFromTop(kControlRowHeight);
    customNamesLabel.setBounds(namesRow.removeFromLeft(120));

    // The "(comma-separated)" hint is secondary: it yields before the editor gets too narrow.
    const bool showHint = namesRow.getWidth() >= 380;
    customNamesHelpLabel.setVisible(showHint);
    if (showHint)
        customNamesHelpLabel.setBounds(namesRow.removeFromRight(140));
    customNamesEditor.setBounds(namesRow);
}

//==============================================================================
int OptionsPanel::getPreferredContentHeight()
{
    // Mirrors resized(): padding, top row, gap, custom-names row, padding.
    return kPadding * 2 + kTopRowHeight + ModernLookAndFeel::Spacing::sm + kNamesRowHeight;
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

void OptionsPanel::applyTo(AudioFileProcessor::ProcessingOptions& options) const
{
    options.customChannelNames = getCustomNames();

    // "48000 Hz" / "24 bit" / "32 bit float" all start with the number. 0 means keep the source.
    options.sampleRate = getOverrideSampleRate() ? getSampleRate().getDoubleValue() : 0.0;
    options.bitDepth = getOverrideBitDepth() ? getBitDepth().getIntValue() : 0;

    options.preserveMetadata = getPreserveIXML();
    options.stereoToMono = getStereoToMono();
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
    sampleRateCombo.addItem("192000 Hz", 6);
    
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
    bitDepthCombo.addItem("32 bit float", 4); // JUCE's WAV writer stores 32-bit output as IEEE float
    
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