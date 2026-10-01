#include "BatchJobManager.h"

//==============================================================================
BatchJobManager::BatchJobManager(int totalJobs, ProgressCallback progressCb, CompletionCallback completionCb)
    : totalJobCount(totalJobs),
      batchStartTime(juce::Time::getCurrentTime()),
      onProgress(std::move(progressCb)),
      onCompletion(std::move(completionCb))
{
    // Initialize progress tracking for each job
    perJobProgress.reserve(static_cast<size_t>(juce::jmax(0, totalJobs)));
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
    if (auto* job = findJob(jobIndex))
    {
        // Set fields on pre-allocated object instead of replacing the pointer
        job->setFilename(filename);
        job->progress.store(0.0);
        job->isActive.store(true);

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " started: " + filename);
        updateProgressDisplay();
    }
}

void BatchJobManager::jobProgress(int jobIndex, double progress)
{
    if (auto* job = findJob(jobIndex))
    {
        // Progress reports reach here through a message-thread hop, so one can arrive after its
        // job already finished; it must not drag a finished job's 100% back down.
        if (!job->isActive.load())
            return;

        job->progress.store(progress);
        updateProgressDisplay();
    }
}

void BatchJobManager::jobCompleted(int jobIndex, double processingTime)
{
    if (auto* job = findJob(jobIndex))
    {
        job->progress.store(1.0);
        job->isActive.store(false);

        {
            std::lock_guard<std::mutex> lock(errorMutex);
            totalProcessingTime += processingTime;
        }
        successfulJobCount.fetch_add(1);

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " completed successfully (" +
                                juce::String(processingTime, 2) + "s)");

        finishJob();
    }
}

void BatchJobManager::jobFailed(int jobIndex, const juce::String& errorMessage)
{
    if (auto* job = findJob(jobIndex))
    {
        job->isActive.store(false);

        {
            std::lock_guard<std::mutex> lock(errorMutex);
            juce::String errorEntry = "File " + juce::String(jobIndex + 1) + ": " + errorMessage;
            collectedErrors.add(errorEntry);
        }
        failedJobCount.fetch_add(1);

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " failed: " + errorMessage);

        finishJob();
    }
}

void BatchJobManager::jobCancelled(int jobIndex)
{
    if (auto* job = findJob(jobIndex))
    {
        job->isActive.store(false);
        cancelledJobCount.fetch_add(1);

        juce::Logger::writeToLog("Job " + juce::String(jobIndex) + " cancelled");

        finishJob();
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

void BatchJobManager::finishJob()
{
    // fetch_add returns the previous value: only the job that brings the count to the total
    // posts the completion, and by then every other job has recorded its own counters.
    const bool isLastJob = completedJobCount.fetch_add(1) + 1 == totalJobCount;

    updateProgressDisplay(); // posted before the completion, so the UI sees "N of N" first

    if (!isLastJob)
        return;

    auto finalResult = buildFinalResult();

    juce::Logger::writeToLog("Batch processing complete: " +
                            juce::String(finalResult.successfulFiles) + " succeeded, " +
                            juce::String(finalResult.failedFiles) + " failed, " +
                            juce::String(finalResult.cancelledFiles) + " cancelled");

    // Completion callback on the message thread. The callback is copied into the lambda so it
    // stays valid even if this manager is destroyed first; the owner is responsible for making
    // the callback itself safe against its own destruction (BatchRun gates it on a liveness flag).
    if (onCompletion)
    {
        auto callback = onCompletion;
        juce::MessageManager::callAsync([callback, finalResult]()
        {
            if (callback)
                callback(finalResult);
        });
    }
}

BatchJobManager::BatchResult BatchJobManager::buildFinalResult() const
{
    BatchResult result;

    result.totalFiles = totalJobCount;
    result.completedFiles = completedJobCount.load();
    result.successfulFiles = successfulJobCount.load();
    result.failedFiles = failedJobCount.load();
    result.cancelledFiles = cancelledJobCount.load();
    result.wasCancelled = cancellationRequested.load();

    // Copy error messages (mutex-protected) and use wall-clock time
    {
        std::lock_guard<std::mutex> lock(errorMutex);
        result.errorMessages = collectedErrors;
    }
    result.totalProcessingTimeSeconds = (juce::Time::getCurrentTime() - batchStartTime).inSeconds();

    return result;
}
