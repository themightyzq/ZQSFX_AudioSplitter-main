#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>
#include "AudioFileProcessor.h"
#include "BatchJobManager.h"
#include "BatchProcessingJob.h"

//==============================================================================
/**
 * BatchRun - one batch: the worker pool, the jobs it runs and the manager they report to,
 * plus the one correct way to take them down.
 *
 * Lifetime rules (this class exists to keep them in one place, with a test):
 * - Workers hold raw pointers to their job and to the manager, so the pool is drained before
 *   either is destroyed. Members are declared so the pool is destroyed first, and the
 *   destructor drains it explicitly before anything else goes away.
 * - Progress and completion callbacks run on the message thread. They are gated on a liveness
 *   flag cleared first thing in the destructor, so a callback already queued when the owner
 *   dies is dropped instead of reaching a destroyed owner.
 * - cancel() is a flag only: it never blocks. Queued jobs drain through it and report
 *   themselves cancelled, so the completion callback still fires exactly once.
 *
 * Construct, use and destroy on the message thread.
 */
class BatchRun
{
public:
    /** Starts immediately: one job per entry of `files`, `workerThreads` at a time. */
    BatchRun(const std::vector<AudioFileProcessor::ProcessingOptions>& files,
             int workerThreads,
             BatchJobManager::ProgressCallback progressCallback,
             BatchJobManager::CompletionCallback completionCallback);

    /** Cancels, waits for every worker to stop, then frees jobs and manager (in that order). */
    ~BatchRun();

    /** Ask every job to stop; returns immediately. Safe to call repeatedly. */
    void cancel();

    bool isCancelRequested() const;

    /** Longest the destructor waits for workers to stop before it deliberately leaks the
        run's memory rather than free what a stuck worker still uses. */
    static constexpr int kDrainTimeoutMs = 20000;

private:
    // Declaration order is destruction order in reverse: the pool must go first.
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
    std::unique_ptr<BatchJobManager> manager;
    juce::OwnedArray<BatchProcessingJob> jobs;
    std::unique_ptr<juce::ThreadPool> pool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchRun)
};
