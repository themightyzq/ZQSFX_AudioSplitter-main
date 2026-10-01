#include "BatchProcessingJob.h"

//==============================================================================
BatchProcessingJob::BatchProcessingJob(const AudioFileProcessor::ProcessingOptions& options,
                                     int index,
                                     BatchJobManager* manager)
    : ThreadPoolJob("AudioProcessing_" + juce::String(index)),
      processingOptions(options),
      jobIndex(index),
      jobManager(manager),
      jobStartTime(juce::Time::getCurrentTime())
{
    // Create independent AudioFileProcessor for this job
    processor = std::make_unique<AudioFileProcessor>();

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " created");
}

BatchProcessingJob::~BatchProcessingJob()
{
    // The owner (BatchRun) has already drained the pool, so no worker is inside runJob(); this
    // only stops a processor thread that somehow outlived it.
    if (processor && processor->isProcessing())
        processor->cancelProcessing();

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " destroyed");
}

//==============================================================================
juce::ThreadPoolJob::JobStatus BatchProcessingJob::runJob()
{
    // A job that starts after a cancel (still queued when the user pressed Cancel) must not
    // touch the disk, but it still has to be reported, or the batch never completes.
    if (jobManager && (jobManager->isCancellationRequested() || shouldExit()))
    {
        jobManager->jobCancelled(jobIndex);
        return jobHasFinished;
    }

    juce::File inputFile(processingOptions.inputFilePath);
    const juce::String filename = inputFile.getFileName();

    if (jobManager)
        jobManager->jobStarted(jobIndex, filename);

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) +
                            " started processing: " + filename);

    // Progress only; cancellation is handled by the polling loop below, never from this
    // message-thread callback (a blocking stop there would freeze the UI).
    auto progressCallback = [this](double progress, const juce::String& message)
    {
        onProgress(progress, message);
    };

    // A batch-wide cancel (or the pool shutting down) reaches the processor at its next audio
    // block, not at the next tick of the polling loop below.
    processor->setExternalCancelCheck([this]
    {
        return (jobManager != nullptr && jobManager->isCancellationRequested()) || shouldExit();
    });

    // Start processing thread, then block until it finishes
    processor->startProcessing(processingOptions, progressCallback, nullptr);

    bool cancelSignalled = false;
    while (processor->isProcessing())
    {
        if (!cancelSignalled && ((jobManager && jobManager->isCancellationRequested()) || shouldExit()))
        {
            // Flag-only: the worker sees it within one audio buffer, deletes its temporary
            // files and reports wasCancelled. Keep waiting for it so the job never outlives
            // (or races) its own processor.
            processor->requestCancel();
            cancelSignalled = true;
        }
        juce::Thread::sleep(20);
    }

    // Thread is done -- read result directly (no message-thread dependency)
    auto jobResult = processor->getResult();

    const double processingTime = (juce::Time::getCurrentTime() - jobStartTime).inSeconds();

    if (jobManager)
    {
        if (jobResult.success)
            jobManager->jobCompleted(jobIndex, processingTime);
        else if (jobResult.wasCancelled)
            jobManager->jobCancelled(jobIndex);
        else
            jobManager->jobFailed(jobIndex, jobResult.errorMessage);
    }

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " finished: " +
                            (jobResult.success ? "SUCCESS" : (jobResult.wasCancelled ? "CANCELLED" : "FAILED")));

    return jobHasFinished;
}

//==============================================================================
// Callbacks

void BatchProcessingJob::onProgress(double progress, const juce::String& message)
{
    // Forward progress to manager
    if (jobManager)
    {
        jobManager->jobProgress(jobIndex, progress);
    }

    // Log detailed progress occasionally (every 10%)
    if (progress - lastLoggedProgress >= 0.1 || progress >= 0.99)
    {
        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " progress: " +
                                juce::String(progress * 100.0, 1) + "% - " + message);
        lastLoggedProgress = progress;
    }
}

void BatchProcessingJob::onCompletion(const AudioFileProcessor::ProcessingResult& result)
{
    // This is called by AudioFileProcessor when processing completes
    // Result is captured by the lambda in runJob()
    juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " completion callback: " +
                            (result.success ? "SUCCESS" : "FAILED"));
}
