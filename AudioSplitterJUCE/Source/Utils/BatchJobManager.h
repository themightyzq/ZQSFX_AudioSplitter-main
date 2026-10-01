#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <vector>

//==============================================================================
/**
 * BatchJobManager - Thread-safe coordinator for parallel batch processing
 *
 * Manages parallel processing of multiple audio files, tracking progress,
 * collecting errors, and coordinating cancellation across all jobs.
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Uses atomic operations for counters
 * - Uses mutex for error collection
 * - Can be called from multiple worker threads simultaneously
 */
class BatchJobManager
{
public:
    //==============================================================================
    struct BatchResult
    {
        int totalFiles {0};
        int completedFiles {0};
        int successfulFiles {0};
        int failedFiles {0};
        int cancelledFiles {0};     // Jobs stopped (or never started) because of a cancel; no output kept
        bool wasCancelled {false};  // True if user cancelled the batch
        juce::StringArray errorMessages;  // List of all errors encountered
        double totalProcessingTimeSeconds {0.0};
    };

    //==============================================================================
    /**
     * Progress callback function type
     * Called on message thread when progress updates
     * @param overallProgress Overall progress 0.0 to 1.0
     * @param currentFile Name of file currently being processed
     * @param completedCount Number of files completed so far
     * @param totalCount Total number of files in batch
     */
    using ProgressCallback = std::function<void(double overallProgress,
                                                const juce::String& currentFile,
                                                int completedCount,
                                                int totalCount)>;

    /**
     * Completion callback function type
     * Called on message thread when all jobs complete
     * @param result Summary of batch processing results
     */
    using CompletionCallback = std::function<void(const BatchResult& result)>;

    //==============================================================================
    BatchJobManager(int totalJobs, ProgressCallback progressCb, CompletionCallback completionCb);
    ~BatchJobManager();

    //==============================================================================
    // Called by worker threads during processing

    /**
     * Mark a job as started
     * Thread-safe: Can be called from any worker thread
     * @param jobIndex Index of the job starting
     * @param filename Name of file being processed
     */
    void jobStarted(int jobIndex, const juce::String& filename);

    /**
     * Update progress for a specific job
     * Thread-safe: Can be called from any worker thread
     * @param jobIndex Index of the job reporting progress
     * @param progress Progress for this job (0.0 to 1.0)
     */
    void jobProgress(int jobIndex, double progress);

    /**
     * Mark a job as successfully completed
     * Thread-safe: Can be called from any worker thread
     * @param jobIndex Index of the completed job
     * @param processingTime Time taken to process (seconds)
     */
    void jobCompleted(int jobIndex, double processingTime);

    /**
     * Mark a job as failed
     * Thread-safe: Can be called from any worker thread
     * @param jobIndex Index of the failed job
     * @param errorMessage Description of the error
     */
    void jobFailed(int jobIndex, const juce::String& errorMessage);

    /**
     * Mark a job as stopped by cancellation (including a job that never started). It counts as
     * finished, so the completion callback still fires exactly once after a cancel.
     * Thread-safe: Can be called from any worker thread
     */
    void jobCancelled(int jobIndex);

    //==============================================================================
    // Called from main thread

    /**
     * Request cancellation of all jobs
     * Thread-safe: Sets atomic flag that worker threads will check
     */
    void cancelAllJobs();

    /**
     * Check if cancellation has been requested
     * Thread-safe: Reads atomic flag
     * @return true if cancellation requested
     */
    bool isCancellationRequested() const;

    /**
     * Get current batch results
     * Thread-safe: Returns copy of result with mutex protection
     * @return Current state of batch processing
     */
    BatchResult getCurrentResult() const;

    /**
     * Get overall progress (0.0 to 1.0)
     * Thread-safe: Uses atomic operations
     * @return Overall progress across all jobs
     */
    double getOverallProgress() const;

private:
    //==============================================================================
    // Job tracking
    const int totalJobCount;
    std::atomic<int> completedJobCount {0};
    std::atomic<int> successfulJobCount {0};
    std::atomic<int> failedJobCount {0};
    std::atomic<int> cancelledJobCount {0};

    // Progress tracking
    struct JobProgress
    {
        std::atomic<double> progress {0.0};
        std::atomic<bool> isActive {false};
        mutable std::mutex filenameMutex;
        juce::String filename;

        JobProgress() = default;

        void setFilename(const juce::String& name)
        {
            std::lock_guard<std::mutex> lock(filenameMutex);
            filename = name;
        }

        juce::String getFilename() const
        {
            std::lock_guard<std::mutex> lock(filenameMutex);
            return filename;
        }
    };
    std::vector<std::unique_ptr<JobProgress>> perJobProgress;  // Pre-allocated, never replaced

    /** The tracking slot for a job index, or null if the index is out of range. */
    JobProgress* findJob(int jobIndex) const
    {
        if (jobIndex < 0 || jobIndex >= static_cast<int>(perJobProgress.size()))
            return nullptr;
        return perJobProgress[static_cast<size_t>(jobIndex)].get();
    }

    // Cancellation flag
    std::atomic<bool> cancellationRequested {false};

    // Error collection and timing (mutex-protected)
    mutable std::mutex errorMutex;
    juce::StringArray collectedErrors;
    double totalProcessingTime {0.0};  // Protected by errorMutex
    juce::Time batchStartTime;

    // Callbacks
    ProgressCallback onProgress;
    CompletionCallback onCompletion;

    //==============================================================================
    // Helper methods

    /**
     * Calculate overall progress across all jobs
     * @return Progress value 0.0 to 1.0
     */
    double calculateOverallProgress() const;

    /**
     * Update progress display on message thread
     * Thread-safe: Uses MessageManager::callAsync
     */
    void updateProgressDisplay();

    /**
     * Count one finished job and, for the job that finishes the batch, post the completion
     * callback. Exactly one caller sees `completed == totalJobCount`, so completion fires once
     * even when the last jobs finish simultaneously. Call only after every other counter and
     * error for this job has been recorded, so the final result is complete.
     */
    void finishJob();

    /**
     * Build final result object
     * Thread-safe: Aggregates atomic counters and mutex-protected errors
     * @return Complete batch result
     */
    BatchResult buildFinalResult() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchJobManager)
};
