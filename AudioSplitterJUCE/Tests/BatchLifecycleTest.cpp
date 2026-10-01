/*
    BatchLifecycleTest
    ------------------
    BatchRun is the object that owns a batch's workers, jobs and manager. This test exercises the
    lifetime rules the old BatchSplitter broke:

      A. Destroying a run while workers are mid-file must not touch freed memory (the old
         destructor was empty, and its members were destroyed in an order that freed the jobs
         under the running workers) and must not deliver a callback afterwards.
      B. A normal run completes once with every file successful, and can be destroyed from
         inside its own completion handling (what BatchSplitter does).
      C. cancel() ends the run: completion fires once with wasCancelled and cancelledFiles.

    The target is built with AddressSanitizer where the compiler supports it, so a
    use-after-free here aborts the test instead of passing by luck.
*/

#include <JuceHeader.h>
#include "BatchRun.h"

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

    // Holds workers mid-file so the destroy and cancel cases never depend on timing: each worker
    // parks in its first audio block (its temporary output exists, nothing finished) until the
    // run it belongs to has been cancelled, which is what ~BatchRun and cancel() both do first.
    struct Hold
    {
        std::atomic<int> held { 0 };
        std::atomic<BatchRun*> run { nullptr };
    };

    std::vector<AudioFileProcessor::ProcessingOptions> makeJobs (const File& root, const String& tag, int count, int seconds,
                                                                 std::shared_ptr<Hold> hold = nullptr)
    {
        const auto in = root.getChildFile ("in_" + tag);  in.createDirectory();
        const auto out = root.getChildFile ("out_" + tag); out.createDirectory();
        std::vector<AudioFileProcessor::ProcessingOptions> jobs;
        for (int i = 0; i < count; ++i)
        {
            const auto src = in.getChildFile ("f" + String (i) + ".wav");
            makeStereoWav (src, seconds);
            AudioFileProcessor::ProcessingOptions o;
            o.inputFilePath = src.getFullPathName();
            o.outputDirectory = out.getFullPathName();
            o.selectedChannels = { 0, 1 };
            if (hold != nullptr)
            {
                o.blockHookForTesting = [hold] (juce::int64 samplesProcessed)
                {
                    if (samplesProcessed != 0)
                        return;
                    ++hold->held;
                    const double end = Time::getMillisecondCounterHiRes() + 30000.0; // only stops a broken build hanging
                    while (Time::getMillisecondCounterHiRes() < end)
                    {
                        auto* run = hold->run.load();
                        if (run != nullptr && run->isCancelRequested())
                            break;
                        Thread::sleep (1);
                    }
                };
            }
            jobs.push_back (o);
        }
        return jobs;
    }

    void pump (int ms) { MessageManager::getInstance()->runDispatchLoopUntil (ms); }

    template <typename Pred>
    bool pumpUntil (Pred done, int timeoutMs)
    {
        const double end = Time::getMillisecondCounterHiRes() + timeoutMs;
        while (! done() && Time::getMillisecondCounterHiRes() < end)
            pump (10);
        return done();
    }
}

int main()
{
    ScopedJuceInitialiser_GUI juceInit;
    std::cout << "=== ZQ SFX BatchLifecycleTest ===" << std::endl;

    const auto root = File::getSpecialLocation (File::tempDirectory).getChildFile ("zqsfx_batch_lifecycle_test");
    root.deleteRecursively();
    root.createDirectory();

    // A. destroy mid-run -----------------------------------------------------------------
    {
        std::cout << "-- destroying a run while workers are held mid-file" << std::endl;
        auto hold = std::make_shared<Hold>();
        const auto jobs = makeJobs (root, "a", 8, 2, hold);

        int progressAfterDestroy = 0, completionsAfterDestroy = 0;
        bool destroyed = false;

        auto run = std::make_unique<BatchRun> (jobs, 4,
            [&] (double, const String&, int, int) { if (destroyed) ++progressAfterDestroy; },
            [&] (const BatchJobManager::BatchResult&) { if (destroyed) ++completionsAfterDestroy; });
        hold->run = run.get();

        while (hold->held.load() < 4)   // all four pool threads are inside a file
            Thread::sleep (1);

        int tempsBefore = 0;
        for (const auto& f : root.getChildFile ("out_a").findChildFiles (File::findFiles, false))
            if (f.getFileName().contains ("_temp")) ++tempsBefore;

        const double t0 = Time::getMillisecondCounterHiRes();
        run.reset();                    // the destructor's cancel is what releases the held workers
        hold->run = nullptr;
        destroyed = true;
        const double ms = Time::getMillisecondCounterHiRes() - t0;

        pump (500); // deliver anything that was queued before the destruction
        check (tempsBefore == 4, "4 temporary outputs existed when the run was destroyed (" + String (tempsBefore) + ")");
        check (progressAfterDestroy == 0 && completionsAfterDestroy == 0,
               "no callback reached the owner after destruction (progress " + String (progressAfterDestroy)
               + ", completion " + String (completionsAfterDestroy) + ")");
        check (ms < 5000.0, "destruction drained the workers promptly (" + String (ms, 0) + " ms)");

        // And no half-written output is left behind by the interrupted jobs.
        check (root.getChildFile ("out_a").getNumberOfChildFiles (File::findFiles) == 0, "no temporary or partial files left by the interrupted jobs");
    }

    // B. normal completion ---------------------------------------------------------------
    {
        std::cout << "-- a normal run" << std::endl;
        const auto jobs = makeJobs (root, "b", 3, 1);

        int completions = 0;
        BatchJobManager::BatchResult result;
        std::unique_ptr<BatchRun> run;
        run = std::make_unique<BatchRun> (jobs, 4,
            nullptr,
            [&] (const BatchJobManager::BatchResult& r)
            {
                ++completions;
                result = r;
                run.reset(); // BatchSplitter destroys the run from inside its completion handling
            });

        const bool finished = pumpUntil ([&] { return completions > 0; }, 20000);
        pump (300);
        check (finished && completions == 1, "completion fired exactly once (" + String (completions) + ")");
        check (result.totalFiles == 3 && result.successfulFiles == 3 && result.failedFiles == 0 && result.cancelledFiles == 0 && ! result.wasCancelled,
               "3 of 3 files successful");
        check (run == nullptr, "run destroyed inside its own completion handler without incident");
        check (root.getChildFile ("out_b").getNumberOfChildFiles (File::findFiles) == 6, "6 mono outputs written");
    }

    // C. cancel --------------------------------------------------------------------------
    {
        std::cout << "-- cancel() with every worker held mid-file" << std::endl;
        auto hold = std::make_shared<Hold>();
        const auto jobs = makeJobs (root, "c", 8, 2, hold);

        std::atomic<int> completions { 0 };
        BatchJobManager::BatchResult result;
        auto run = std::make_unique<BatchRun> (jobs, 4, nullptr,
            [&] (const BatchJobManager::BatchResult& r) { result = r; ++completions; });
        hold->run = run.get();

        while (hold->held.load() < 4)
            Thread::sleep (1);

        run->cancel();
        const bool finished = pumpUntil ([&] { return completions.load() > 0; }, 30000);
        pump (300);

        check (finished && completions == 1, "completion fired exactly once after cancel (" + String (completions.load()) + ")");
        check (result.wasCancelled, "wasCancelled set");
        check (result.cancelledFiles == 8 && result.successfulFiles == 0 && result.failedFiles == 0,
               "all 8 files counted as cancelled (" + String (result.cancelledFiles) + ")");
        check (root.getChildFile ("out_c").getNumberOfChildFiles (File::findFiles) == 0, "no temporary or partial files left");
        hold->run = nullptr;
        run.reset();
    }

    root.deleteRecursively();

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
