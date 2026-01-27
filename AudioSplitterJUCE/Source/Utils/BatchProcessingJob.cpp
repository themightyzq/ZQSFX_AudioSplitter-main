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
    // Ensure processor is stopped before destruction
    if (processor && processor->isProcessing())
    {
        processor->cancelProcessing();
        // Give it a moment to clean up
        juce::Thread::sleep(100);
    }

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " destroyed");
}

//==============================================================================
juce::ThreadPoolJob::JobStatus BatchProcessingJob::runJob()
{
    // Extract filename for display
    juce::File inputFile(processingOptions.inputFilePath);
    const juce::String filename = inputFile.getFileName();

    // Notify manager that this job is starting
    if (jobManager)
    {
        jobManager->jobStarted(jobIndex, filename);
    }

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) +
                            " started processing: " + filename);

    // Set up callbacks for progress and completion
    bool jobCompleted = false;
    AudioFileProcessor::ProcessingResult jobResult;

    auto progressCallback = [this](double progress, const juce::String& message)
    {
        // Check for cancellation
        if (jobManager && jobManager->isCancellationRequested())
        {
            if (processor)
            {
                processor->cancelProcessing();
            }
            return;
        }

        onProgress(progress, message);
    };

    auto completionCallback = [&jobCompleted, &jobResult](const AudioFileProcessor::ProcessingResult& result)
    {
        jobResult = result;
        jobCompleted = true;
    };

    // Start processing (this blocks until completion)
    processor->startProcessing(processingOptions, progressCallback, completionCallback);

    // Wait for completion callback to be invoked
    // AudioFileProcessor runs on its own thread and will call completionCallback
    // We need to wait for it to finish
    const int maxWaitTime = 60000; // 60 seconds max per file
    const int checkInterval = 100;  // Check every 100ms
    int waitedTime = 0;

    while (!jobCompleted && waitedTime < maxWaitTime)
    {
        // Check for cancellation
        if (jobManager && jobManager->isCancellationRequested())
        {
            juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " cancelled");

            if (processor)
            {
                processor->cancelProcessing();
            }

            return jobHasFinished;  // Return finished (cancelled is a type of finish)
        }

        if (shouldExit())
        {
            juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " should exit");

            if (processor)
            {
                processor->cancelProcessing();
            }

            return jobHasFinished;
        }

        juce::Thread::sleep(checkInterval);
        waitedTime += checkInterval;
    }

    // Check if we timed out
    if (!jobCompleted)
    {
        juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " TIMEOUT after " +
                                juce::String(maxWaitTime / 1000) + " seconds");

        if (processor)
        {
            processor->cancelProcessing();
        }

        // Report timeout as failure
        if (jobManager)
        {
            jobManager->jobFailed(jobIndex, "Processing timeout (>" + juce::String(maxWaitTime / 1000) + "s)");
        }

        return jobHasFinished;
    }

    // Calculate processing time
    const double processingTime = (juce::Time::getCurrentTime() - jobStartTime).inSeconds();

    // Report result to manager
    if (jobManager)
    {
        if (jobResult.success)
        {
            jobManager->jobCompleted(jobIndex, processingTime);
        }
        else
        {
            jobManager->jobFailed(jobIndex, jobResult.errorMessage);
        }
    }

    juce::Logger::writeToLog("BatchProcessingJob " + juce::String(jobIndex) + " finished: " +
                            (jobResult.success ? "SUCCESS" : "FAILED"));

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
    static double lastLoggedProgress = 0.0;
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
