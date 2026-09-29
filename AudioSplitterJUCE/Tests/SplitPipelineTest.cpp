/*
    SplitPipelineTest  (end-to-end)
    -------------------------------
    RECLAMATION_BACKLOG item 12. Drives the REAL splitting pipeline -- AudioFileProcessor,
    which internally calls extractChannel -> JUCE writer -> BroadcastChunkPreserver -- on a
    multichannel fixture, then asserts that each split MONO output actually carries the
    classic iXML metadata. This closes the gap left by MetadataRoundTripTest, which exercises
    the preserver in isolation rather than through the processor.

    Runs headless: AudioFileProcessor only touches the MessageManager when progress/completion
    callbacks are set, so we pass none and drive it synchronously via waitForResult().
*/

#include <JuceHeader.h>
#include "AudioFileProcessor.h"
#include "TestWavUtil.h"

using namespace juce;

int main()
{
    int failures = 0;
    auto fail = [&] (const String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; };
    auto ok   = [&] (const String& msg) { std::cout << "  [ ok ] " << msg << std::endl; };

    std::cout << "=== ZQ SFX SplitPipelineTest (end-to-end) ===" << std::endl;

    // ---- 1. Build a 2-channel fixture with bext + classic iXML ----
    const auto tempDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("zqsfx_pipeline_test");
    tempDir.deleteRecursively();
    tempDir.createDirectory();
    const auto srcFile = tempDir.getChildFile ("multichannel_source.wav");
    const auto outDir  = tempDir.getChildFile ("out");
    outDir.createDirectory();

    const MemoryBlock fixture = testwav::buildFixtureWav (2000); // 2000 frames
    if (! srcFile.replaceWithData (fixture.getData(), fixture.getSize()))
        { fail ("could not write fixture"); return 1; }

    int64 srcLen = 0;
    {
        WavAudioFormat wav;
        std::unique_ptr<AudioFormatReader> r (wav.createReaderFor (new FileInputStream (srcFile), true));
        if (r == nullptr) { fail ("fixture not readable as WAV"); return 1; }
        srcLen = r->lengthInSamples;
        if (r->numChannels != 2) fail ("fixture should be 2 channels, got " + String (r->numChannels));
    }
    ok ("fixture: 2ch / " + String (srcLen) + " samples, with bext + classic iXML");

    // ---- 2. Run the real pipeline: split both channels to mono files ----
    AudioFileProcessor::ProcessingOptions options;
    options.inputFilePath   = srcFile.getFullPathName();
    options.outputDirectory = outDir.getFullPathName();
    options.selectedChannels = { 0, 1 };
    options.preserveMetadata = true;

    AudioFileProcessor processor;
    processor.startProcessing (options);                 // null callbacks -> no MessageManager needed
    const auto result = processor.waitForResult (30000);  // synchronous wait

    if (! result.success) { fail ("pipeline failed: " + result.errorMessage); return 1; }
    if (result.outputFiles.size() != 2)
        fail ("expected 2 output files, got " + String (result.outputFiles.size()));
    else
        ok ("pipeline produced 2 mono files: " + result.outputFiles.joinIntoString (", "));

    // ---- 3. Each split mono output must be a valid mono WAV carrying classic iXML ----
    WavAudioFormat wav;
    for (const auto& path : result.outputFiles)
    {
        const File out (path);
        const String name = out.getFileName();

        if (! out.existsAsFile()) { fail (name + ": output file missing"); continue; }

        MemoryBlock bytes; out.loadFileAsData (bytes);
        const String ixml = testwav::findChunkText (bytes, "iXML");

        // (a) classic iXML present and complete
        if (! testwav::hasChunk (bytes, "iXML"))
        {
            fail (name + ": iXML chunk absent (preservation did not run through the pipeline)");
        }
        else
        {
            int lost = 0;
            for (const auto& f : testwav::classicFields())
                if (! testwav::ixmlFieldPresent (ixml, f)) { fail (name + ": iXML missing " + f); ++lost; }
            if (lost == 0) ok (name + ": all classic iXML fields present");
        }

        // (b) bext preserved
        if (testwav::hasChunk (bytes, "bext")
            && testwav::findChunkText (bytes, "bext").contains (testwav::bextDescription()))
            ok (name + ": bext preserved");
        else
            fail (name + ": bext missing/damaged");

        // (c) valid MONO WAV with full audio length
        std::unique_ptr<AudioFormatReader> r (wav.createReaderFor (new FileInputStream (out), true));
        if (r == nullptr)                       fail (name + ": not a readable WAV");
        else if (r->numChannels != 1)           fail (name + ": expected mono, got " + String (r->numChannels) + " ch");
        else if (r->lengthInSamples != srcLen)  fail (name + ": audio length " + String (r->lengthInSamples)
                                                       + " != source " + String (srcLen));
        else                                    ok (name + ": valid mono WAV, audio intact");
    }

    // ---- 4. Failure path must still fire the completion callback exactly once ----
    // Regression: run()'s early returns (unreadable input, bad output dir, write failure,
    // cancel) used to skip completeProcessing(), so single-file mode never reported the error.
    {
        ScopedJuceInitialiser_GUI juceInit; // completion is delivered via MessageManager::callAsync

        AudioFileProcessor::ProcessingOptions badOptions;
        badOptions.inputFilePath    = tempDir.getChildFile ("does_not_exist.wav").getFullPathName();
        badOptions.outputDirectory  = outDir.getFullPathName();
        badOptions.selectedChannels = { 0 };

        int completions = 0;
        AudioFileProcessor::ProcessingResult completionResult;

        AudioFileProcessor failing;
        failing.startProcessing (badOptions, nullptr,
                                 [&] (const AudioFileProcessor::ProcessingResult& r)
                                 { ++completions; completionResult = r; });

        // Bounded wait: fails (rather than hangs) if the callback never fires.
        const auto deadline = Time::getMillisecondCounterHiRes() + 5000.0;
        while (completions == 0 && Time::getMillisecondCounterHiRes() < deadline)
            MessageManager::getInstance()->runDispatchLoopUntil (20);

        // Keep pumping briefly so a duplicate callback would be caught.
        MessageManager::getInstance()->runDispatchLoopUntil (300);

        if (completions != 1)
            fail ("failed run: completion callback fired " + String (completions) + " time(s), expected exactly 1");
        else if (completionResult.success)
            fail ("failed run: completion result reported success");
        else if (completionResult.errorMessage.isEmpty())
            fail ("failed run: completion result has empty error message");
        else
            ok ("failed run: completion fired once with error: " + completionResult.errorMessage);
    }

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
