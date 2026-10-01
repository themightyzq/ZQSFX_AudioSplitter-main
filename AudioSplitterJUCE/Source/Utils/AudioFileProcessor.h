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

        // Only acts on a 2-channel source with both channels selected: the pair is mixed
        // ((L + R) / 2) into ONE mono file named "<source>_mono" (UCS suffix "M") instead of
        // being split into two. Any other source or selection is split per channel as usual.
        bool stereoToMono {false};

        // Test seam, empty in the app: called on the worker thread before every audio block with
        // the number of samples written so far. A test blocks in it to hold a run mid-file at a
        // known point (output temp file created, nothing finished) so cancel and teardown tests
        // never depend on timing.
        std::function<void(juce::int64 samplesProcessed)> blockHookForTesting;

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
        bool wasCancelled {false};              // true when stopped by cancelProcessing()/requestCancel()
        juce::String errorMessage;
        juce::StringArray outputFiles;          // only files that were fully written, verified and moved into place
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
     * Ask the worker to stop and return immediately (safe on the message thread). The worker
     * notices within one audio buffer, removes its temporary files and reports a result with
     * wasCancelled == true. Existing output files are never touched by a cancelled run.
     */
    void requestCancel();

    /**
     * Optional extra stop condition, polled with the processor's own cancel flag at every audio
     * block. BatchProcessingJob uses it so a batch-wide cancel takes effect at the next block
     * instead of waiting for the job's polling loop. Set before startProcessing(); the callable
     * runs on the worker thread.
     */
    void setExternalCancelCheck(std::function<bool()> check) { externalCancelCheck = std::move(check); }

    /**
     * requestCancel() and then block (up to 3 s) until the worker thread has stopped. Not for the
     * message thread; used from teardown paths that must know the thread is gone.
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
    std::function<bool()> externalCancelCheck;

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
    
    /** One output file: a single source channel, or (mixPartnerChannel >= 0) the mix of two. */
    struct OutputPlan
    {
        int sourceChannel {0};
        int mixPartnerChannel {-1};
        juce::File target;
    };

    /**
     * Decide what files this run writes and what they are called (standard, custom-name, UCS
     * or stereo-to-mono naming). Returns false and sets the error if two outputs would collide
     * or an output would overwrite the source file.
     */
    bool buildOutputPlans(std::vector<OutputPlan>& plans);

    /**
     * Process every planned output. Two-phase so a failure never leaves a half-replaced set:
     * (1) write and verify each output into a temporary file beside its target, (2) only when
     * every temporary is complete, swap them over the targets. Temporaries are removed on any
     * failure or cancel; existing files are only replaced by a finished, verified file.
     */
    bool processChannels();

    /**
     * Write one output into `destination` (a temporary file), checking every write, then
     * re-open it and verify format, channel count and length.
     */
    bool writeOutputFile(const OutputPlan& plan, const juce::File& destination);

    bool isCancelRequested() const
    {
        return threadShouldExit() || shouldCancel.load() || (externalCancelCheck && externalCancelCheck());
    }

    /** Record a cancel: error text plus wasCancelled, so callers can tell it from a failure. */
    void setCancelled();
    
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