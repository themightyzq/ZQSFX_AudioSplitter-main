#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Audio Analyzer - mirrors Python FFprobe functionality
 * 
 * Python equivalent: update_channel_checkboxes() (lines 1016-1048)
 * Responsible for:
 * - Audio file validation and format detection
 * - Channel count detection using JUCE AudioFormatManager
 * - Audio file properties analysis
 * - File format validation (WAV files)
 */
class AudioAnalyzer
{
public:
    //==============================================================================
    struct AudioFileInfo
    {
        bool isValid {false};
        int numChannels {0};
        double sampleRate {0.0};
        int bitDepth {0};
        juce::int64 lengthInSamples {0};
        double lengthInSeconds {0.0};
        juce::String formatName;
        juce::String errorMessage;
    };

    //==============================================================================
    AudioAnalyzer();
    ~AudioAnalyzer();

    //==============================================================================
    /**
     * Analyze audio file - mirrors Python FFprobe functionality
     * @param filePath Path to audio file to analyze
     * @return AudioFileInfo structure with file details
     */
    AudioFileInfo analyzeFile(const juce::String& filePath);
    
    /**
     * Quick channel count check - mirrors Python channel detection
     * @param filePath Path to audio file
     * @return Number of channels, or -1 if invalid
     */
    int getChannelCount(const juce::String& filePath);
    
    /**
     * Validate if file is a supported audio format
     * @param filePath Path to file to check
     * @return true if file is valid WAV format
     */
    bool isValidAudioFile(const juce::String& filePath);
    
    /**
     * Check if file exists and is accessible
     * @param filePath Path to check
     * @return true if file exists and can be read
     */
    bool fileExists(const juce::String& filePath);

private:
    //==============================================================================
    juce::AudioFormatManager formatManager;
    
    //==============================================================================
    // Helper methods
    void setupFormatManager();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};