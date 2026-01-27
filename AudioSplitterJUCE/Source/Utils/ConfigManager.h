#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Configuration manager - mirrors Python config loading/saving system
 * 
 * Python equivalent: 
 * - load_config() function (lines 614-627)
 * - save_config() function (lines 629-637)
 * - CONFIG_FILE constant (line 612)
 * - Global directory variables (lines 200-202)
 * 
 * Responsible for:
 * - Loading/saving user preferences
 * - Managing last used directories
 * - Application settings persistence
 */
class ConfigManager
{
public:
    //==============================================================================
    ConfigManager();
    ~ConfigManager();

    //==============================================================================
    // Configuration management - mirrors Python config functions
    
    /**
     * Load configuration from file - mirrors Python load_config() (lines 614-627)
     */
    void loadConfig();
    
    /**
     * Save configuration to file - mirrors Python save_config() (lines 629-637)
     */
    void saveConfig();

    //==============================================================================
    // Directory management - mirrors Python global directory variables
    
    /**
     * Get last input directory - mirrors Python last_input_dir (line 200)
     */
    juce::String getLastInputDir() const { return lastInputDir; }
    
    /**
     * Set last input directory - mirrors Python last_input_dir updates
     */
    void setLastInputDir(const juce::String& dir);
    
    /**
     * Get last output directory - mirrors Python last_output_dir (line 201)
     */
    juce::String getLastOutputDir() const { return lastOutputDir; }
    
    /**
     * Set last output directory - mirrors Python last_output_dir updates
     */
    void setLastOutputDir(const juce::String& dir);
    
    /**
     * Get last general directory - mirrors Python last_dir (line 202)
     */
    juce::String getLastDir() const { return lastDir; }
    
    /**
     * Set last general directory - mirrors Python last_dir updates
     */
    void setLastDir(const juce::String& dir);

    //==============================================================================
    // Application settings - mirrors Python UI state variables
    
    /**
     * Get default sample rate - mirrors Python sample_rate_var default (line 1219)
     */
    juce::String getDefaultSampleRate() const { return defaultSampleRate; }
    
    /**
     * Set default sample rate
     */
    void setDefaultSampleRate(const juce::String& rate);
    
    /**
     * Get default bit depth - mirrors Python bit_depth_var default (line 1221)
     */
    juce::String getDefaultBitDepth() const { return defaultBitDepth; }
    
    /**
     * Set default bit depth
     */
    void setDefaultBitDepth(const juce::String& depth);
    
    /**
     * Get default custom names - mirrors Python custom_names_var default (line 1213)
     */
    juce::String getDefaultCustomNames() const { return defaultCustomNames; }
    
    /**
     * Set default custom names
     */
    void setDefaultCustomNames(const juce::String& names);

private:
    //==============================================================================
    // Configuration file management
    
    /**
     * Get config file path - mirrors Python CONFIG_FILE (line 612)
     */
    juce::File getConfigFile();
    
    /**
     * Get application root directory - mirrors Python get_application_root() (lines 95-100)
     */
    juce::File getApplicationRoot();

    //==============================================================================
    // Configuration data - mirrors Python global variables
    
    // Directory settings (mirrors Python lines 200-202)
    juce::String lastInputDir;
    juce::String lastOutputDir;
    juce::String lastDir;
    
    // Application settings (mirrors Python default values)
    juce::String defaultSampleRate {"48000 Hz"}; // mirrors line 1219
    juce::String defaultBitDepth {"16 bit"};     // mirrors line 1221  
    juce::String defaultCustomNames {"L,R,C,lfe,Ls,Rs,Lss,Rss"}; // mirrors line 1213
    
    // Internal state
    bool configLoaded {false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigManager)
};