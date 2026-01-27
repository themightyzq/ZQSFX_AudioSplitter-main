#pragma once

#include <JuceHeader.h>
#include "AudioFileProcessor.h"
#include "BatchJobManager.h"

//==============================================================================
/**
 * BatchProcessingJob - Wrapper for AudioFileProcessor to run in ThreadPool
 *
 * Inherits from juce::ThreadPoolJob to integrate with JUCE's ThreadPool system.
 * Each job processes one audio file independently in parallel with other jobs.
 *
 * Thread Safety:
 * - Each job operates on independent AudioFileProcessor instance
 * - Reports progress/completion to thread-safe BatchJobManager
 * - Can be cancelled via BatchJobManager
 */
class BatchProcessingJob : public juce::ThreadPoolJob
{
public:
    //==============================================================================
    /**
     * Create a batch processing job
     * @param options Audio processing configuration
     * @param jobIndex Index of this job in the batch (for tracking)
     * @param manager Batch job manager for progress/completion reporting
     */
    BatchProcessingJob(const AudioFileProcessor::ProcessingOptions& options,
                      int jobIndex,
                      BatchJobManager* manager);

    ~BatchProcessingJob() override;

    //==============================================================================
    // ThreadPoolJob interface

    /**
     * Execute the job on a worker thread
     * @return Result code indicating success/cancellation
     */
    JobStatus runJob() override;

private:
    //==============================================================================
    // Job state
    const AudioFileProcessor::ProcessingOptions processingOptions;
    const int jobIndex;
    BatchJobManager* jobManager;  // Non-owning pointer (managed by BatchSplitter)

    // Each job has its own processor instance (thread-safe independence)
    std::unique_ptr<AudioFileProcessor> processor;

    // Timing
    juce::Time jobStartTime;

    //==============================================================================
    // Callbacks for AudioFileProcessor

    /**
     * Progress callback - forwards to BatchJobManager
     * @param progress Progress for this job (0.0 to 1.0)
     * @param message Status message (logged)
     */
    void onProgress(double progress, const juce::String& message);

    /**
     * Completion callback - reports result to BatchJobManager
     * @param result Processing result (success/error)
     */
    void onCompletion(const AudioFileProcessor::ProcessingResult& result);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchProcessingJob)
};
