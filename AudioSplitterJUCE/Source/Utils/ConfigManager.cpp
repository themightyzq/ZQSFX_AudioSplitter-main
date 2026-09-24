#include "ConfigManager.h"

//==============================================================================
ConfigManager::ConfigManager()
{
    // Initialize with default values - mirrors Python default initialization
    // This mirrors the pattern used in Python where defaults are set before loading
    
    // Set default directories to home directory - mirrors Python os.path.expanduser("~")
    auto homeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
    lastInputDir = homeDir;
    lastOutputDir = homeDir;
    lastDir = homeDir;
    
    juce::Logger::writeToLog("ConfigManager initialized with defaults");
}

//==============================================================================
ConfigManager::~ConfigManager()
{
    // Save config on destruction if needed
    if (configLoaded)
    {
        saveConfig();
    }
}

//==============================================================================
void ConfigManager::loadConfig()
{
    // Mirror Python load_config() function exactly (lines 614-627)
    
    auto configFile = getConfigFile();
    
    if (configFile.existsAsFile())
    {
        try
        {
            // Read JSON config file - mirrors Python json.load()
            auto configText = configFile.loadFileAsString();
            auto configJson = juce::JSON::parse(configText);
            
            if (configJson.isObject())
            {
                auto* jsonObject = configJson.getDynamicObject();
                
                // Load directory settings - mirrors Python config.get() calls (lines 620-621)
                if (jsonObject->hasProperty("last_input_dir"))
                {
                    lastInputDir = jsonObject->getProperty("last_input_dir").toString();
                }
                
                if (jsonObject->hasProperty("last_output_dir"))
                {
                    lastOutputDir = jsonObject->getProperty("last_output_dir").toString();
                }
                
                // Load additional settings
                if (jsonObject->hasProperty("default_sample_rate"))
                {
                    defaultSampleRate = jsonObject->getProperty("default_sample_rate").toString();
                }
                
                if (jsonObject->hasProperty("default_bit_depth"))
                {
                    defaultBitDepth = jsonObject->getProperty("default_bit_depth").toString();
                }
                
                if (jsonObject->hasProperty("default_custom_names"))
                {
                    defaultCustomNames = jsonObject->getProperty("default_custom_names").toString();
                }

                // Load UI state
                if (jsonObject->hasProperty("ui_state"))
                {
                    auto uiStateVar = jsonObject->getProperty("ui_state");
                    if (auto* uiObj = uiStateVar.getDynamicObject())
                    {
                        for (const auto& prop : uiObj->getProperties())
                            uiState[prop.name.toString()] = prop.value.toString();
                    }
                }

                juce::Logger::writeToLog("Loaded config: " + configText);
            }
        }
        catch (const std::exception& e)
        {
            // Mirror Python exception handling (lines 623-625)
            juce::String errorMsg = "Error loading config: " + juce::String(e.what());
            juce::Logger::writeToLog(errorMsg);
        }
    }
    
    // Mirror Python fallback logic (lines 626-627)
    if (lastOutputDir.isEmpty())
    {
        lastOutputDir = lastInputDir;
    }
    
    // Update lastDir to match the most recent directory
    if (!lastInputDir.isEmpty())
    {
        lastDir = lastInputDir;
    }
    
    configLoaded = true;
}

//==============================================================================
void ConfigManager::saveConfig()
{
    // Mirror Python save_config() function exactly (lines 629-637)
    
    try
    {
        // Create JSON object - mirrors Python config dict (line 630)
        juce::DynamicObject::Ptr configObject = new juce::DynamicObject();
        
        // Set directory properties - mirrors Python config dict creation
        configObject->setProperty("last_input_dir", lastInputDir);
        configObject->setProperty("last_output_dir", lastOutputDir);
        
        // Set additional settings
        configObject->setProperty("default_sample_rate", defaultSampleRate);
        configObject->setProperty("default_bit_depth", defaultBitDepth);
        configObject->setProperty("default_custom_names", defaultCustomNames);

        // Save UI state
        if (!uiState.empty())
        {
            juce::DynamicObject::Ptr uiObj = new juce::DynamicObject();
            for (const auto& [key, value] : uiState)
                uiObj->setProperty(key, value);
            configObject->setProperty("ui_state", juce::var(uiObj.get()));
        }

        // Convert to JSON string
        juce::var configVar(configObject.get());
        juce::String configText = juce::JSON::toString(configVar);
        
        // Write to file - mirrors Python json.dump() (lines 632-633)
        auto configFile = getConfigFile();
        configFile.replaceWithText(configText);
        
        juce::Logger::writeToLog("Saved config: " + configText);
    }
    catch (const std::exception& e)
    {
        // Mirror Python exception handling (lines 635-637)
        juce::String errorMsg = "Error saving config: " + juce::String(e.what());
        juce::Logger::writeToLog(errorMsg);
    }
}

//==============================================================================
void ConfigManager::setLastInputDir(const juce::String& dir)
{
    lastInputDir = dir;
    lastDir = dir; // Update general last directory
}

//==============================================================================
void ConfigManager::setLastOutputDir(const juce::String& dir)
{
    lastOutputDir = dir;
    lastDir = dir; // Update general last directory
}

//==============================================================================
void ConfigManager::setLastDir(const juce::String& dir)
{
    lastDir = dir;
}

//==============================================================================
void ConfigManager::setDefaultSampleRate(const juce::String& rate)
{
    defaultSampleRate = rate;
}

//==============================================================================
void ConfigManager::setDefaultBitDepth(const juce::String& depth)
{
    defaultBitDepth = depth;
}

//==============================================================================
void ConfigManager::setDefaultCustomNames(const juce::String& names)
{
    defaultCustomNames = names;
}

//==============================================================================
juce::String ConfigManager::getUIState(const juce::String& key, const juce::String& defaultValue) const
{
    auto it = uiState.find(key);
    return (it != uiState.end()) ? it->second : defaultValue;
}

void ConfigManager::setUIState(const juce::String& key, const juce::String& value)
{
    uiState[key] = value;
}

//==============================================================================
juce::File ConfigManager::getConfigFile()
{
    // Mirror Python CONFIG_FILE creation (line 612)
    // CONFIG_FILE = os.path.join(get_application_root(), "config.json")
    
    auto appRoot = getApplicationRoot();
    return appRoot.getChildFile("config.json");
}

//==============================================================================
juce::File ConfigManager::getApplicationRoot()
{
    // Mirror Python get_application_root() function (lines 95-100)
    
    // For now, use the executable's directory
    // In Python, this checks for PyInstaller frozen state, but in JUCE we just use executable location
    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    return executableFile.getParentDirectory();
}