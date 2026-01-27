#pragma once

#include <JuceHeader.h>
#include <functional>
#include <atomic>
#include <memory>

//==============================================================================
/**
 * Audio File Processor - Core audio file splitting functionality
 * 
 * Python equivalent: split_single_file() function (lines 673-840)
 * Responsible for:
 * - Loading audio files using JUCE AudioFormatManager
 * - Extracting individual channels from multichannel audio
 * - Writing channel files with proper metadata preservation
 * - Progress reporting during processing
 * - Background thread execution
 */
class AudioFileProcessor : public juce::Thread
{
public:
    //==============================================================================
    struct ProcessingOptions
    {
        juce::String inputFilePath;
        juce::String outputDirectory;
        std::vector<int> selectedChannels;      // Which channels to extract (0-based)
        juce::String customChannelNames;        // Comma-separated custom names
        double sampleRate {0.0};                // 0 = keep original
        int bitDepth {0};                       // 0 = keep original
        bool preserveMetadata {true};
        bool stereoToMono {false};

        // UCS naming options
        bool useUCSNaming {false};
        juce::String ucsCategory;               // e.g., "AMBNat"
        juce::String ucsSubcategory;            // e.g., "Forest"
        juce::String ucsDescription;            // e.g., "MorningBirds"
        juce::StringArray ucsChannelSuffixes;   // e.g., ["L", "R"] or auto-generate
    };

    struct ProcessingResult
    {
        bool success {false};
        juce::String errorMessage;
        juce::StringArray outputFiles;
        double processingTimeSeconds {0.0};
        juce::int64 totalSamplesProcessed {0};
    };

    //==============================================================================
    /**
     * Progress callback function type
     * @param progress Float from 0.0 to 1.0
     * @param statusMessage Current operation description
     */
    using ProgressCallback = std::function<void(double progress, const juce::String& statusMessage)>;

    /**
     * Completion callback function type
     * @param result Processing result with success/error info
     */
    using CompletionCallback = std::function<void(const ProcessingResult& result)>;

    //==============================================================================
    AudioFileProcessor();
    ~AudioFileProcessor() override;

    //==============================================================================
    /**
     * Start audio processing - mirrors Python split_single_file()
     * @param options Processing configuration
     * @param progressCallback Called periodically with progress updates
     * @param completionCallback Called when processing completes or fails
     */
    void startProcessing(const ProcessingOptions& options,
                        ProgressCallback progressCallback = nullptr,
                        CompletionCallback completionCallback = nullptr);

    /**
     * Cancel processing - mirrors user cancellation in Python
     */
    void cancelProcessing();

    /**
     * Check if processing is currently active
     */
    bool isProcessing() const { return isThreadRunning(); }

    /**
     * Wait for processing to finish and return the result.
     * For synchronous use (e.g., batch processing from a thread pool).
     * @param timeoutMs Maximum wait time in milliseconds (-1 = infinite)
     */
    ProcessingResult waitForResult(int timeoutMs = -1);

    /**
     * Get a thread-safe copy of the current result.
     */
    ProcessingResult getResult() const;

    //==============================================================================
    // Thread interface
    void run() override;

private:
    //==============================================================================
    // Processing state
    ProcessingOptions currentOptions;
    ProgressCallback onProgress;
    CompletionCallback onCompletion;

    // Thread-safe result access
    mutable std::mutex resultMutex;
    ProcessingResult result;

    // Progress tracking
    std::atomic<double> currentProgress {0.0};
    std::atomic<bool> shouldCancel {false};

    // Audio processing infrastructure
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReader> audioReader;

    // Prevent dangling this in async callbacks: shared flag set to false on destruction
    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);
    
    //==============================================================================
    // Core processing methods - mirror Python implementation
    
    /**
     * Load and validate input audio file - mirrors Python file loading (lines 686-702)
     */
    bool loadInputFile();
    
    /**
     * Create output directory if needed - mirrors Python directory creation (lines 704-709)
     */
    bool setupOutputDirectory();
    
    /**
     * Generate output filenames - mirrors Python filename generation (lines 716-732)
     */
    juce::StringArray generateOutputFilenames();
    
    /**
     * Process each selected channel - mirrors Python channel extraction (lines 734-820)
     */
    bool processChannels();
    
    /**
     * Extract single channel to file - mirrors Python single channel processing
     * @param channelIndex 0-based channel index
     * @param outputPath Output file path
     * @param channelName Custom name for the channel
     */
    bool extractChannel(int channelIndex, const juce::String& outputPath, const juce::String& channelName);
    
    /**
     * Apply audio format options - mirrors Python format conversion
     * @param reader Source audio reader
     * @param writer Destination audio writer
     */
    void applyFormatOptions(juce::AudioFormatReader* reader, juce::AudioFormatWriter* writer);
    
    /**
     * Update progress safely from processing thread
     * @param progress Progress value 0.0-1.0
     * @param message Status message
     */
    void updateProgress(double progress, const juce::String& message);
    
    /**
     * Complete processing and notify main thread
     */
    void completeProcessing();

    /**
     * Thread-safe error setting
     */
    void setError(const juce::String& errorMessage);

    /**
     * Thread-safe result field access
     */
    void addOutputFile(const juce::String& filepath);
    void incrementSamplesProcessed(juce::int64 samples);
    void setSuccess(bool success);
    void setProcessingTime(double seconds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileProcessor)
};