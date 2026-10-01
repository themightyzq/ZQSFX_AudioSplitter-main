/*
    BatchCancelTest
    ---------------
    Cancelling a batch must still finish the batch: the completion callback fires exactly once
    with wasCancelled set, no half-written files are left, and nothing hangs.

    The old code returned from a cancelled job without telling the BatchJobManager, so the
    completion callback never fired and a cancelled batch never ended (the UI stayed busy
    forever). This test drives BatchJobManager + BatchProcessingJob + a ThreadPool directly,
    exactly as BatchRun does.

    No timing assumptions. A run is held at a known point instead of being raced:
      - "before start": cancel is requested before any job is submitted;
      - "held mid-file": every worker blocks (via ProcessingOptions::blockHookForTesting) after
        its temporary output exists and before any audio block is written, until the test has
        requested the cancel, so no job can finish first;
      - "from a progress callback": the same hold, but the cancel is requested from inside the
        batch's progress callback on the message thread, as the Cancel button's handler would.
    The scenarios marked OLD-API-BEGIN/END use only API that existed before the fix, so the
    "before start" case can be compiled against the old code to prove it fails there.
*/

#include <JuceHeader.h>
#include "AudioFileProcessor.h"
#include "BatchJobManager.h"
#include "BatchProcessingJob.h"

#include <atomic>

using namespace juce;

namespace
{
    int failures = 0;
    void fail (const String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; }
    void ok   (const String& msg) { std::cout << "  [ ok ] " << msg << std::endl; }
    void check (bool condition, const String& msg) { if (condition) ok (msg); else fail (msg); }

    void makeStereoWav (const File& file, int seconds)
    {
        WavAudioFormat wav;
        const int frames = 48000 * seconds;
        std::unique_ptr<AudioFormatWriter> writer (wav.createWriterFor (new FileOutputStream (file), 48000.0, 2, 16, {}, 0));
        AudioBuffer<float> buffer (2, frames);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < frames; ++i)
                buffer.setSample (c, i, 0.25f * (float) std::sin (0.01 * i + c));
        writer->writeFromAudioSampleBuffer (buffer, 0, frames);
    }

    std::vector<File> makeSources (const File& dir, int count, int seconds)
    {
        dir.createDirectory();
        std::vector<File> sources;
        for (int i = 0; i < count; ++i)
        {
            sources.push_back (dir.getChildFile ("f" + String (i) + ".wav"));
            makeStereoWav (sources.back(), seconds);
        }
        return sources;
    }

    StringArray names (const File& dir)
    {
        StringArray n;
        for (const auto& f : dir.findChildFiles (File::findFiles, false)) n.add (f.getFileName());
        n.sort (false);
        return n;
    }

    void pump (int ms) { MessageManager::getInstance()->runDispatchLoopUntil (ms); }

    template <typename Pred>
    bool pumpUntil (Pred done, int timeoutMs)
    {
        const double end = Time::getMillisecondCounterHiRes() + timeoutMs;
        while (! done() && Time::getMillisecondCounterHiRes() < end)
            pump (5);
        return done();
    }

    // State shared with callbacks that may run after runBatch() returns (queued message-thread
    // calls), so it is owned by a shared_ptr, never by a stack frame.
    struct RunOutcome
    {
        std::atomic<int> completions { 0 };
        BatchJobManager::BatchResult result;
        std::atomic<int> workersHeld { 0 };      // jobs currently parked in the hold
        std::atomic<BatchJobManager*> manager { nullptr };
        int tempFilesWhileHeld = -1;             // files in the output folder while the workers were held
    };

    enum class CancelMode { beforeStart, heldFromTest, heldFromProgressCallback };

    // Runs the sources through manager + jobs + pool. Teardown order matters: pool, jobs, manager.
    void runBatch (const std::vector<File>& sources, const File& outDir, CancelMode mode, std::shared_ptr<RunOutcome> outcome)
    {
        std::unique_ptr<BatchJobManager> manager;
        OwnedArray<BatchProcessingJob> jobs;
        std::unique_ptr<ThreadPool> pool;

        // Runs on the message thread whenever a job reports progress. In the "progress callback"
        // mode it is the one thing that requests the cancel, and only once every worker is parked.
        // The disk is looked at here, just before the cancel, because the held workers are only
        // guaranteed to still be parked until this callback requests it.
        const String outPath = outDir.getFullPathName();
        auto progress = [outcome, mode, outPath] (double, const String&, int, int)
        {
            if (mode == CancelMode::heldFromProgressCallback && outcome->workersHeld.load() >= 4)
            {
                if (auto* m = outcome->manager.load())
                {
                    if (! m->isCancellationRequested())
                        outcome->tempFilesWhileHeld = File (outPath).getNumberOfChildFiles (File::findFiles);
                    m->cancelAllJobs(); // what the Cancel button's handler does
                }
            }
        };

        manager = std::make_unique<BatchJobManager> ((int) sources.size(),
            BatchJobManager::ProgressCallback (progress),
            [outcome] (const BatchJobManager::BatchResult& r) { outcome->result = r; ++outcome->completions; });
        outcome->manager = manager.get();

        if (mode == CancelMode::beforeStart)
            manager->cancelAllJobs();

        pool = std::make_unique<ThreadPool> (4);
        for (size_t i = 0; i < sources.size(); ++i)
        {
            AudioFileProcessor::ProcessingOptions o;
            o.inputFilePath = sources[i].getFullPathName();
            o.outputDirectory = outDir.getFullPathName();
            o.selectedChannels = { 0, 1 };

            if (mode != CancelMode::beforeStart)
            {
                // Park in the first block until the cancel has been requested. The timeout only
                // keeps a broken build from hanging; a healthy run never reaches it.
                BatchJobManager* managerPtr = manager.get();
                o.blockHookForTesting = [managerPtr, outcome] (juce::int64 samplesProcessed)
                {
                    if (samplesProcessed != 0)
                        return;
                    ++outcome->workersHeld;
                    const double end = Time::getMillisecondCounterHiRes() + 30000.0;
                    while (! managerPtr->isCancellationRequested() && Time::getMillisecondCounterHiRes() < end)
                        Thread::sleep (1);
                };
            }

            auto* job = new BatchProcessingJob (o, (int) i, manager.get());
            jobs.add (job);
            pool->addJob (job, false);
        }

        if (mode != CancelMode::beforeStart)
        {
            // Wait until every pool thread is parked mid-file, then look at the disk.
            // (Only wait here: in the progress-callback mode a pending report from a job can run
            // the cancel as soon as the 4th worker parks.)
            while (outcome->workersHeld.load() < 4)
                Thread::sleep (1);

            if (mode == CancelMode::heldFromTest)
            {
                outcome->tempFilesWhileHeld = outDir.getNumberOfChildFiles (File::findFiles);
                manager->cancelAllJobs();
            }
            else
            {
                manager->jobProgress (0, 0.5); // a progress report from a running job; its callback cancels
            }
        }

        pumpUntil ([&] { return outcome->completions.load() > 0; }, 30000);
        pump (300); // keep pumping so a second completion would be caught

        pool->removeAllJobs (true, 30000);
        pool.reset();
        jobs.clear();
        outcome->manager = nullptr;
        manager.reset();
        pump (50);  // flush any callback still queued from the jobs
    }
}

int main()
{
    ScopedJuceInitialiser_GUI juceInit;
    std::cout << "=== ZQ SFX BatchCancelTest ===" << std::endl;

    const auto root = File::getSpecialLocation (File::tempDirectory).getChildFile ("zqsfx_batch_cancel_test");
    root.deleteRecursively();
    root.createDirectory();

    // OLD-API-BEGIN
    {
        std::cout << "-- cancel requested before any job starts" << std::endl;
        const auto sources = makeSources (root.getChildFile ("in_a"), 6, 2);
        const auto out = root.getChildFile ("out_a"); out.createDirectory();

        auto outcome = std::make_shared<RunOutcome>();
        runBatch (sources, out, CancelMode::beforeStart, outcome);
        check (outcome->completions == 1, "completion fired exactly once (" + String (outcome->completions.load()) + ")");
        check (outcome->result.wasCancelled, "result reports the batch was cancelled");
        check (outcome->result.totalFiles == 6, "result covers all 6 files");
        check (names (out).isEmpty(), "nothing was written (" + names (out).joinIntoString (", ") + ")");
    }
    // OLD-API-END

    for (auto mode : { CancelMode::heldFromTest, CancelMode::heldFromProgressCallback })
    {
        const bool fromTest = mode == CancelMode::heldFromTest;
        const String tag = fromTest ? "b" : "c";
        std::cout << (fromTest ? "-- cancel while every worker is held mid-file (requested by the test)"
                               : "-- cancel while every worker is held mid-file (requested from the progress callback)") << std::endl;

        const auto sources = makeSources (root.getChildFile ("in_" + tag), 8, 2);
        const auto out = root.getChildFile ("out_" + tag); out.createDirectory();

        auto outcome = std::make_shared<RunOutcome>();
        runBatch (sources, out, mode, outcome);

        check (outcome->workersHeld >= 4, "4 workers were parked mid-file when the cancel was requested");
        check (outcome->tempFilesWhileHeld == 4, "their 4 temporary outputs existed at that moment (" + String (outcome->tempFilesWhileHeld) + ")");
        check (outcome->completions == 1, "completion fired exactly once (" + String (outcome->completions.load()) + ")");
        check (outcome->result.wasCancelled, "result reports the batch was cancelled");
        check (outcome->result.totalFiles == 8 && outcome->result.cancelledFiles == 8
               && outcome->result.successfulFiles == 0 && outcome->result.failedFiles == 0,
               "all 8 files counted as cancelled (ok " + String (outcome->result.successfulFiles) + ", failed "
               + String (outcome->result.failedFiles) + ", cancelled " + String (outcome->result.cancelledFiles) + ")");
        check (names (out).isEmpty(), "every temporary and partial file was removed (" + names (out).joinIntoString (", ") + ")");
    }

    root.deleteRecursively();

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
