#include "BatchJobManager.h"

//==============================================================================
BatchJobManager::BatchJobManager(int totalJobs, ProgressCallback progressCb, CompletionCallback completionCb)
    : totalJobCount(totalJobs),
      onProgress(std::move(progressCb)),
      onCompletion(std::move(completionCb)),
      batchStartTime(juce::Time::getCurrentTime())
{
    // Initialize progress tracking for each job
    perJobProgress.reserve(totalJobs);
    for (int i = 0; i < totalJobs; ++i)
    {
        perJobProgress.push_back(std::make_unique<JobProgress>());
    }

    juce::Logger::writeToLog("BatchJobManager created for " + juce::String(totalJobs) + " jobs");
}

BatchJobManager::~BatchJobManager()
{
    juce::Logger::writeToLog("BatchJobManager destroyed");
}

//==============================================================================
// Worker thread methods

void BatchJobManager::jobStarted(int jobIndex, const juce::String& filename)
{
    if (jobIndex >= 0 && jobIndex < static_cast<int>(perJobProgress.size()))
    {
        // Set fields on pre-allocated object instead of replacing the pointer
        perJobProgress[jobIndex]->setFilename(filename);
        perJobProgress[jobIndex]->progress.store(0.0);
        perJobProgress[jobIndex]->isActive.store(true);

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " started: " + filename);
        updateProgressDisplay();
    }
}

void BatchJobManager::jobProgress(int jobIndex, double progress)
{
    if (jobIndex >= 0 && jobIndex < static_cast<int>(perJobProgress.size()))
    {
        perJobProgress[jobIndex]->progress.store(progress);
        updateProgressDisplay();
    }
}

void BatchJobManager::jobCompleted(int jobIndex, double processingTime)
{
    if (jobIndex >= 0 && jobIndex < static_cast<int>(perJobProgress.size()))
    {
        perJobProgress[jobIndex]->progress.store(1.0);
        perJobProgress[jobIndex]->isActive.store(false);

        // Update counters atomically
        completedJobCount.fetch_add(1);
        successfulJobCount.fetch_add(1);

        // Add processing time (mutex-protected)
        {
            std::lock_guard<std::mutex> lock(errorMutex);
            totalProcessingTime += processingTime;
        }

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " completed successfully (" +
                                juce::String(processingTime, 2) + "s)");

        updateProgressDisplay();
        checkBatchCompletion();
    }
}

void BatchJobManager::jobFailed(int jobIndex, const juce::String& errorMessage)
{
    if (jobIndex >= 0 && jobIndex < static_cast<int>(perJobProgress.size()))
    {
        perJobProgress[jobIndex]->isActive.store(false);

        // Update counters atomically
        completedJobCount.fetch_add(1);
        failedJobCount.fetch_add(1);

        // Add error message (mutex-protected)
        {
            std::lock_guard<std::mutex> lock(errorMutex);
            juce::String errorEntry = "File " + juce::String(jobIndex + 1) + ": " + errorMessage;
            collectedErrors.add(errorEntry);
        }

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " failed: " + errorMessage);

        updateProgressDisplay();
        checkBatchCompletion();
    }
}

//==============================================================================
// Cancellation

void BatchJobManager::cancelAllJobs()
{
    cancellationRequested.store(true);
    juce::Logger::writeToLog("Cancellation requested for all jobs");
}

bool BatchJobManager::isCancellationRequested() const
{
    return cancellationRequested.load();
}

//==============================================================================
// Status queries

BatchJobManager::BatchResult BatchJobManager::getCurrentResult() const
{
    return buildFinalResult();
}

double BatchJobManager::getOverallProgress() const
{
    return calculateOverallProgress();
}

//==============================================================================
// Private helper methods

double BatchJobManager::calculateOverallProgress() const
{
    if (totalJobCount == 0)
        return 1.0;

    // Sum up progress from all jobs
    double totalProgress = 0.0;
    for (const auto& job : perJobProgress)
    {
        totalProgress += job->progress.load();
    }

    return totalProgress / static_cast<double>(totalJobCount);
}

void BatchJobManager::updateProgressDisplay()
{
    // Find currently active file (for display purposes)
    juce::String currentFile;
    for (const auto& job : perJobProgress)
    {
        if (job->isActive.load())
        {
            currentFile = job->getFilename();
            break;  // Show first active file
        }
    }

    const double overallProgress = calculateOverallProgress();
    const int completed = completedJobCount.load();
    const int total = totalJobCount;

    // Call progress callback on message thread
    // NOTE: Capture by value to avoid lifetime issues if this object is destroyed
    if (onProgress)
    {
        auto callback = onProgress;  // Copy callback to ensure it stays valid
        juce::MessageManager::callAsync([callback, overallProgress, currentFile, completed, total]()
        {
            if (callback)
            {
                callback(overallProgress, currentFile, completed, total);
            }
        });
    }
}

void BatchJobManager::checkBatchCompletion()
{
    const int completed = completedJobCount.load();

    if (completed >= totalJobCount)
    {
        // All jobs complete - build final result and notify
        auto finalResult = buildFinalResult();

        juce::Logger::writeToLog("Batch processing complete: " +
                                juce::String(finalResult.successfulFiles) + " succeeded, " +
                                juce::String(finalResult.failedFiles) + " failed");

        // Call completion callback on message thread
        // NOTE: Capture callback by value to avoid lifetime issues
        if (onCompletion)
        {
            auto callback = onCompletion;  // Copy callback to ensure it stays valid
            juce::MessageManager::callAsync([callback, finalResult]()
            {
                if (callback)
                {
                    callback(finalResult);
                }
            });
        }
    }
}

BatchJobManager::BatchResult BatchJobManager::buildFinalResult() const
{
    BatchResult result;

    result.totalFiles = totalJobCount;
    result.completedFiles = completedJobCount.load();
    result.successfulFiles = successfulJobCount.load();
    result.failedFiles = failedJobCount.load();
    result.wasCancelled = cancellationRequested.load();

    // Copy error messages (mutex-protected) and use wall-clock time
    {
        std::lock_guard<std::mutex> lock(errorMutex);
        result.errorMessages = collectedErrors;
    }
    result.totalProcessingTimeSeconds = (juce::Time::getCurrentTime() - batchStartTime).inSeconds();

    return result;
}
