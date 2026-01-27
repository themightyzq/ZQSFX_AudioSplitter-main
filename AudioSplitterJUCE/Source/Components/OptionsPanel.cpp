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
    setupSampleRateOptions();
    sampleRateGroup.setColour(juce::GroupComponent::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(sampleRateGroup);
    
    // Override checkbox - mirrors Python lines 1577-1589
    overrideSampleRateToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    overrideSampleRateToggle.onClick = [this]() { toggleSampleRateDropdown(); };
    addAndMakeVisible(overrideSampleRateToggle);
    
    sampleRateLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(sampleRateLabel);
    
    sampleRateCombo.onChange = [this]() { settingsChanged(); };
    sampleRateCombo.setEnabled(false); // Initially disabled - mirrors Python line 87
    sampleRateCombo.setColour(juce::ComboBox::backgroundColourId, ModernLookAndFeel::Colors::surface);
    sampleRateCombo.setColour(juce::ComboBox::textColourId, ModernLookAndFeel::Colors::textPrimary);
    sampleRateCombo.setColour(juce::ComboBox::outlineColourId, ModernLookAndFeel::Colors::border);
    addAndMakeVisible(sampleRateCombo);
    
    // Setup bit depth options - mirrors Python lines 1594-1610
    setupBitDepthOptions();
    bitDepthGroup.setColour(juce::GroupComponent::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(bitDepthGroup);
    
    // Override checkbox - mirrors Python lines 1602-1614
    overrideBitDepthToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    overrideBitDepthToggle.onClick = [this]() { toggleBitDepthDropdown(); };
    addAndMakeVisible(overrideBitDepthToggle);
    
    bitDepthLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(bitDepthLabel);
    
    bitDepthCombo.onChange = [this]() { settingsChanged(); };
    bitDepthCombo.setEnabled(false); // Initially disabled - mirrors Python line 93
    bitDepthCombo.setColour(juce::ComboBox::backgroundColourId, ModernLookAndFeel::Colors::surface);
    bitDepthCombo.setColour(juce::ComboBox::textColourId, ModernLookAndFeel::Colors::textPrimary);
    bitDepthCombo.setColour(juce::ComboBox::outlineColourId, ModernLookAndFeel::Colors::border);
    addAndMakeVisible(bitDepthCombo);
    
    // Setup custom names - mirrors Python lines 1612-1628
    setupCustomNames();
    customNamesGroup.setColour(juce::GroupComponent::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(customNamesGroup);
    
    customNamesLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(customNamesLabel);
    
    customNamesHelpLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(customNamesHelpLabel);
    
    customNamesEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    customNamesEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    customNamesEditor.onTextChange = [this]() { settingsChanged(); };
    addAndMakeVisible(customNamesEditor);
    
    // Setup options toggles - mirrors Python lines 1630-1701
    setupOptionsToggles();
    optionsGroup.setColour(juce::GroupComponent::textColourId, ModernLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(optionsGroup);
    
    stereoToMonoToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    stereoToMonoToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(stereoToMonoToggle);
    
    preserveIXMLToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
    preserveIXMLToggle.onClick = [this]() { settingsChanged(); };
    addAndMakeVisible(preserveIXMLToggle);
    
    channelRemappingToggle.setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);
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
    const int spacing = ModernLookAndFeel::Spacing::sm;  // 8px

    // Tight vertical padding to maximize content area
    bounds.reduce(spacing, spacing);

    // Top section: 3 columns for Sample Rate, Bit Depth, Options groups
    // Fixed height accommodates: title(20) + padding(16) + toggle(25) + gap(4) + label(20) + gap(4) + combo(30) + padding = 127
    const int groupHeight = 130;
    auto topSection = bounds.removeFromTop(groupHeight);

    auto leftCol = topSection.removeFromLeft(topSection.getWidth() / 3);
    auto midCol = topSection.removeFromLeft(topSection.getWidth() / 2);
    auto rightCol = topSection;

    // Sample rate group - left column
    sampleRateGroup.setBounds(leftCol);
    auto srInner = leftCol;
    srInner.removeFromTop(20); // group title space
    srInner = srInner.reduced(spacing, spacing);
    overrideSampleRateToggle.setBounds(srInner.removeFromTop(25));
    srInner.removeFromTop(4);
    sampleRateLabel.setBounds(srInner.removeFromTop(20));
    srInner.removeFromTop(4);
    sampleRateCombo.setBounds(srInner.removeFromTop(30));

    // Bit depth group - middle column
    bitDepthGroup.setBounds(midCol);
    auto bdInner = midCol;
    bdInner.removeFromTop(20);
    bdInner = bdInner.reduced(spacing, spacing);
    overrideBitDepthToggle.setBounds(bdInner.removeFromTop(25));
    bdInner.removeFromTop(4);
    bitDepthLabel.setBounds(bdInner.removeFromTop(20));
    bdInner.removeFromTop(4);
    bitDepthCombo.setBounds(bdInner.removeFromTop(30));

    // Options group - right column
    optionsGroup.setBounds(rightCol);
    auto optInner = rightCol;
    optInner.removeFromTop(20);
    optInner = optInner.reduced(spacing, spacing);
    constexpr int toggleHeight = 28;  // WCAG 2.5.8 minimum target size
    stereoToMonoToggle.setBounds(optInner.removeFromTop(toggleHeight));
    preserveIXMLToggle.setBounds(optInner.removeFromTop(toggleHeight));
    channelRemappingToggle.setBounds(optInner.removeFromTop(toggleHeight));

    bounds.removeFromTop(spacing);

    // Bottom section: Custom Channel Names - flows below top groups, no overlap
    customNamesGroup.setBounds(bounds);
    auto cnInner = bounds;
    cnInner.removeFromTop(20); // group title space
    cnInner = cnInner.reduced(spacing, spacing / 2);
    auto cnRow = cnInner.removeFromTop(28);
    customNamesLabel.setBounds(cnRow.removeFromLeft(120));
    customNamesHelpLabel.setBounds(cnRow.removeFromRight(120));
    customNamesEditor.setBounds(cnRow);
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