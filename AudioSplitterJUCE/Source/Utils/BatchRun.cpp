#include "BatchRun.h"

//==============================================================================
BatchRun::BatchRun(const std::vector<AudioFileProcessor::ProcessingOptions>& files,
                   int workerThreads,
                   BatchJobManager::ProgressCallback progressCallback,
                   BatchJobManager::CompletionCallback completionCallback)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Both callbacks are posted to the message thread by the manager; drop them once this run
    // is gone so they can never reach a destroyed owner.
    auto liveness = alive;
    manager = std::make_unique<BatchJobManager>(
        (int) files.size(),
        [liveness, progressCallback](double progress, const juce::String& currentFile, int completed, int total)
        {
            if (liveness->load() && progressCallback)
                progressCallback(progress, currentFile, completed, total);
        },
        [liveness, completionCallback](const BatchJobManager::BatchResult& result)
        {
            if (liveness->load() && completionCallback)
                completionCallback(result);
        });

    pool = std::make_unique<juce::ThreadPool>(juce::jmax(1, workerThreads));

    for (size_t i = 0; i < files.size(); ++i)
    {
        auto* job = new BatchProcessingJob(files[i], (int) i, manager.get());
        jobs.add(job);
        pool->addJob(job, false); // false: we own the job (see ~BatchRun for the order)
    }
}

//==============================================================================
BatchRun::~BatchRun()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // 1. No callback may reach the owner from here on.
    alive->store(false);

    // 2. Stop the work. Jobs poll this flag, ask their processor to stop and wait for it.
    if (manager != nullptr)
        manager->cancelAllJobs();

    // 3. Wait until no worker is inside a job. Only then is it safe to free the jobs and the
    //    manager they point at.
    if (pool != nullptr && !pool->removeAllJobs(true, kDrainTimeoutMs))
    {
        // A worker is stuck (e.g. blocked in I/O on a vanished drive). Freeing what it still
        // uses would be a use-after-free, so leak this run instead and say so.
        juce::Logger::writeToLog("BatchRun: workers did not stop in time; leaking the run to stay memory-safe");
        pool.release();
        manager.release();
        while (jobs.size() > 0)
            jobs.remove(jobs.size() - 1, false);
        return;
    }

    pool.reset();
    jobs.clear();
    manager.reset();
}

//==============================================================================
void BatchRun::cancel()
{
    if (manager != nullptr)
        manager->cancelAllJobs();
}

bool BatchRun::isCancelRequested() const
{
    return manager != nullptr && manager->isCancellationRequested();
}
