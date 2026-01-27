#pragma once

#include <JuceHeader.h>
#include "../Utils/ConfigManager.h"

//==============================================================================
/**
 * Options Panel Component - mirrors Python options_frame
 * 
 * Python equivalent: Lines 1560-1701 (Options Frame setup)
 * Responsible for:
 * - Sample rate selection (mirrors lines 1576-1592)
 * - Bit depth selection (mirrors lines 1594-1610)
 * - Custom channel naming (mirrors lines 1612-1628)
 * - Stereo to mono conversion options (mirrors lines 1630-1673)
 * - iXML metadata preservation (mirrors lines 1675-1691)
 * - Channel remapping options (mirrors lines 1693-1701)
 */
class OptionsPanel : public juce::Component
{
public:
    //==============================================================================
    explicit OptionsPanel(ConfigManager* configManager);
    ~OptionsPanel() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Settings access - mirrors Python variable access
    
    /**
     * Get if sample rate override is enabled - mirrors Python override_sample_rate_var.get()
     */
    bool getOverrideSampleRate() const;
    
    /**
     * Get selected sample rate - mirrors Python sample_rate_var.get() (line 1219)
     */
    juce::String getSampleRate() const;
    
    /**
     * Get if bit depth override is enabled - mirrors Python override_bit_depth_var.get()
     */
    bool getOverrideBitDepth() const;
    
    /**
     * Get selected bit depth - mirrors Python bit_depth_var.get() (line 1221)
     */
    juce::String getBitDepth() const;
    
    /**
     * Get custom channel names - mirrors Python custom_names_var.get() (line 1213)
     */
    juce::String getCustomNames() const;
    
    /**
     * Get stereo to mono option - mirrors Python stereo_to_mono_var.get() (line 1215)
     */
    bool getStereoToMono() const;
    
    /**
     * Get iXML preservation option - mirrors Python preserve_ixml_var.get() (line 1217)
     */
    bool getPreserveIXML() const;
    
    /**
     * Get channel remapping option - mirrors Python channel_remapping_var.get() (line 1225)
     */
    bool getChannelRemapping() const;

    //==============================================================================
    // Callback for settings changes - mirrors Python variable tracing
    std::function<void()> onSettingsChanged;

private:
    //==============================================================================
    // UI Components - mirrors Python options_frame structure
    
    // Sample rate selection - mirrors lines 1576-1592
    juce::GroupComponent sampleRateGroup;
    juce::ToggleButton overrideSampleRateToggle;  // mirrors override_sample_rate_var
    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;
    
    // Bit depth selection - mirrors lines 1594-1610
    juce::GroupComponent bitDepthGroup;
    juce::ToggleButton overrideBitDepthToggle;    // mirrors override_bit_depth_var
    juce::Label bitDepthLabel;
    juce::ComboBox bitDepthCombo;
    
    // Custom channel naming - mirrors lines 1612-1628
    juce::GroupComponent customNamesGroup;
    juce::Label customNamesLabel;
    juce::TextEditor customNamesEditor;
    juce::Label customNamesHelpLabel;
    
    // Options checkboxes - mirrors lines 1630-1701
    juce::GroupComponent optionsGroup;
    juce::ToggleButton stereoToMonoToggle;
    juce::ToggleButton preserveIXMLToggle;
    juce::ToggleButton channelRemappingToggle;
    
    //==============================================================================
    // State management
    ConfigManager* configManager;
    
    //==============================================================================
    // Helper methods
    void setupSampleRateOptions();   // mirrors Python sample rate setup (lines 1578-1592)
    void setupBitDepthOptions();     // mirrors Python bit depth setup (lines 1596-1610)
    void setupCustomNames();         // mirrors Python custom names setup (lines 1614-1628)
    void setupOptionsToggles();      // mirrors Python checkbox setup (lines 1630-1701)
    void settingsChanged();          // triggers callback when settings change
    void toggleSampleRateDropdown(); // mirrors Python toggle_sample_rate_dropdown (line 83)
    void toggleBitDepthDropdown();   // mirrors Python toggle_bit_depth_dropdown (line 89)
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OptionsPanel)
};