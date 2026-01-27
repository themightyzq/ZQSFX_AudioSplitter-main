#include "AudioAnalyzer.h"

//==============================================================================
AudioAnalyzer::AudioAnalyzer()
{
    setupFormatManager();
    juce::Logger::writeToLog("AudioAnalyzer initialized");
}

//==============================================================================
AudioAnalyzer::~AudioAnalyzer()
{
}

//==============================================================================
AudioAnalyzer::AudioFileInfo AudioAnalyzer::analyzeFile(const juce::String& filePath)
{
    AudioFileInfo info;
    
    // Check if file exists first - mirrors Python os.path.isfile() check (line 1020)
    if (!fileExists(filePath))
    {
        info.errorMessage = "File does not exist or is not accessible";
        juce::Logger::writeToLog("File analysis failed: " + info.errorMessage);
        return info;
    }
    
    juce::File audioFile(filePath);
    
    try
    {
        // Create audio format reader - mirrors Python FFprobe functionality
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
        
        if (reader == nullptr)
        {
            info.errorMessage = "Unsupported audio format or corrupted file";
            juce::Logger::writeToLog("File analysis failed: " + info.errorMessage);
            return info;
        }
        
        // Extract audio properties - mirrors Python FFprobe output parsing
        info.isValid = true;
        info.numChannels = (int)reader->numChannels;          // mirrors line 1034
        info.sampleRate = reader->sampleRate;
        info.bitDepth = (int)reader->bitsPerSample;
        info.lengthInSamples = reader->lengthInSamples;
        info.lengthInSeconds = (reader->sampleRate > 0.0)
            ? reader->lengthInSamples / reader->sampleRate
            : 0.0;
        info.formatName = reader->getFormatName();
        
        juce::Logger::writeToLog("File analyzed successfully: " + filePath);
        juce::Logger::writeToLog("  Channels: " + juce::String(info.numChannels));
        juce::Logger::writeToLog("  Sample Rate: " + juce::String(info.sampleRate, 1) + " Hz");
        juce::Logger::writeToLog("  Bit Depth: " + juce::String(info.bitDepth) + " bit");
        juce::Logger::writeToLog("  Length: " + juce::String(info.lengthInSeconds, 2) + " seconds");
        
        return info;
    }
    catch (const std::exception& e)
    {
        info.errorMessage = "Error reading audio file: " + juce::String(e.what());
        juce::Logger::writeToLog("File analysis exception: " + info.errorMessage);
        return info;
    }
}

//==============================================================================
int AudioAnalyzer::getChannelCount(const juce::String& filePath)
{
    // Quick channel count check - mirrors Python FFprobe command (lines 1025-1034)
    auto info = analyzeFile(filePath);
    return info.isValid ? info.numChannels : -1;
}

//==============================================================================
bool AudioAnalyzer::isValidAudioFile(const juce::String& filePath)
{
    // Validate file format - mirrors Python WAV file filtering
    if (!fileExists(filePath))
        return false;
        
    // Check file extension first (mirrors Python filetypes filter)
    juce::File file(filePath);
    if (!file.getFileExtension().toLowerCase().equalsIgnoreCase(".wav"))
    {
        juce::Logger::writeToLog("File rejected: not a WAV file - " + filePath);
        return false;
    }
    
    // Try to create a reader to validate the file
    juce::File audioFile(filePath);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(audioFile));
    
    bool isValid = (reader != nullptr);
    if (!isValid)
    {
        juce::Logger::writeToLog("File validation failed: cannot create reader - " + filePath);
    }
    
    return isValid;
}

//==============================================================================
bool AudioAnalyzer::fileExists(const juce::String& filePath)
{
    // Mirror Python os.path.isfile() check (line 1020)
    juce::File file(filePath);
    bool exists = file.existsAsFile();
    
    if (!exists)
    {
        juce::Logger::writeToLog("File does not exist: " + filePath);
    }
    
    return exists;
}

//==============================================================================
void AudioAnalyzer::setupFormatManager()
{
    // Register audio formats - focus on WAV format for 1:1 Python parity
    formatManager.registerBasicFormats();
    
    juce::Logger::writeToLog("Audio format manager initialized with basic formats");
}