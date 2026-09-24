/*
    MetadataRoundTripTest  (unit)
    -----------------------------
    Executable test for ZQ SFX Audio Splitter (RECLAMATION_BACKLOG items 1-3).

    Builds a deterministic WAV fixture (see TestWavUtil.h) with full ground-truth control
    over the `bext` (BWF) and classic `iXML` chunks (SCENE/TAKE/TAPE/PROJECT/TRACK_LIST with
    per-channel track names), then verifies two paths by raw-parsing the output RIFF:

      PART A (characterization): round-trip through juce::WavAudioFormat alone
        (read metadataValues -> createWriterFor). Documents the root defect: JUCE 8 drops
        classic iXML entirely. Reported, not asserted.

      PART B (the fix, HARD assertions): apply BroadcastChunkPreserver::preserve() to the
        JUCE output -- exactly what AudioFileProcessor does after writing each channel --
        and require that bext, audio, AND every classic iXML field survive.

    This file exercises the JUCE metadata path and the preservation routine directly; the
    end-to-end pipeline (AudioFileProcessor driving both) is covered by SplitPipelineTest.
*/

#include <juce_audio_formats/juce_audio_formats.h>
#include "BroadcastChunkPreserver.h"
#include "TestWavUtil.h"

using namespace juce;

int main()
{
    int failures = 0;
    auto fail = [&] (const String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; };
    auto ok   = [&] (const String& msg) { std::cout << "  [ ok ] " << msg << std::endl; };

    std::cout << "=== ZQ SFX MetadataRoundTripTest ===" << std::endl;

    // ---- 1. Build + persist fixture ----
    const auto tempDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("zqsfx_meta_test");
    tempDir.createDirectory();
    const auto srcFile = tempDir.getChildFile ("fixture_source.wav");
    const auto outFile = tempDir.getChildFile ("fixture_roundtrip.wav");
    outFile.deleteFile();

    const MemoryBlock fixture = testwav::buildFixtureWav();
    if (! srcFile.replaceWithData (fixture.getData(), fixture.getSize()))
        { fail ("could not write fixture to " + srcFile.getFullPathName()); return 1; }

    // ---- 2. Sanity: source fixture must contain everything (HARD invariant) ----
    MemoryBlock srcBytes; srcFile.loadFileAsData (srcBytes);
    const String srcIxml = testwav::findChunkText (srcBytes, "iXML");
    if (! testwav::hasChunk (srcBytes, "bext")) fail ("fixture missing bext");
    if (! testwav::hasChunk (srcBytes, "iXML")) fail ("fixture missing iXML");
    for (const auto& t : testwav::classicFields())
        if (! testwav::ixmlFieldPresent (srcIxml, t)) fail ("fixture iXML missing " + t);
    if (failures > 0) { std::cerr << "Fixture is malformed; aborting." << std::endl; return 1; }
    ok ("fixture built with bext + classic iXML (SCENE/TAKE/TAPE/PROJECT/TRACK_LIST)");

    // ---- 3. Round-trip through JUCE exactly like AudioFileProcessor's writer ----
    WavAudioFormat wav;
    std::unique_ptr<AudioFormatReader> reader (wav.createReaderFor (new FileInputStream (srcFile), true));
    if (reader == nullptr) { fail ("JUCE could not read fixture"); return 1; }

    const int64 srcLen = reader->lengthInSamples;
    const StringPairArray md = reader->metadataValues; // what the splitter passes through

    {
        std::unique_ptr<FileOutputStream> outStream (outFile.createOutputStream());
        if (outStream == nullptr) { fail ("could not open output stream"); return 1; }
        auto* raw = outStream.get();
        std::unique_ptr<AudioFormatWriter> writer (
            wav.createWriterFor (raw, reader->sampleRate, reader->numChannels,
                                 (int) reader->bitsPerSample, md, 0));
        if (writer == nullptr) { fail ("JUCE could not create writer"); return 1; }
        outStream.release(); // writer owns the stream now
        if (! writer->writeFromAudioReader (*reader, 0, srcLen))
            fail ("writeFromAudioReader failed");
    } // writer flushed + closed

    // ---- 4. Inspect JUCE-only output ----
    MemoryBlock outBytes; outFile.loadFileAsData (outBytes);
    const String outIxml = testwav::findChunkText (outBytes, "iXML");

    // 4a. HARD invariant: bext Description must survive (this is the part that works).
    if (testwav::hasChunk (outBytes, "bext")
        && testwav::findChunkText (outBytes, "bext").contains (testwav::bextDescription()))
        ok ("bext preserved (Description round-tripped)");
    else
        fail ("REGRESSION: bext Description lost on round-trip");

    // 4b. HARD invariant: audio sample count preserved.
    {
        std::unique_ptr<AudioFormatReader> outReader (wav.createReaderFor (new FileInputStream (outFile), true));
        if (outReader == nullptr) fail ("could not re-read output");
        else if (outReader->lengthInSamples != srcLen)
            fail ("audio length changed: " + String (srcLen) + " -> " + String (outReader->lengthInSamples));
        else
            ok ("audio sample count preserved (" + String (srcLen) + " samples)");
    }

    // ---- PART A: characterization -- what JUCE alone does (reported, not asserted) ----
    std::cout << "\n--- PART A: classic iXML survival through JUCE alone (the defect) ---" << std::endl;
    std::cout << "    iXML chunk present in JUCE output: " << (testwav::hasChunk (outBytes, "iXML") ? "yes" : "NO") << std::endl;
    int juceLost = 0;
    for (const auto& f : testwav::classicFields())
        if (testwav::ixmlFieldPresent (srcIxml, f) && ! testwav::ixmlFieldPresent (outIxml, f)) ++juceLost;
    std::cout << "    classic fields lost by JUCE: " << juceLost << "/" << testwav::classicFields().size()
              << (juceLost > 0 ? "   (root defect -> BroadcastChunkPreserver required)" : "") << std::endl;

    // ---- PART B: the fix -- apply BroadcastChunkPreserver, then HARD-assert survival ----
    std::cout << "\n--- PART B: after BroadcastChunkPreserver::preserve() (the fix) ---" << std::endl;
    String preserveMsg;
    const bool preserveOk = BroadcastChunkPreserver::preserve (srcFile, outFile, preserveMsg);
    std::cout << "    preserve() -> " << (preserveOk ? "ok" : "FALSE") << ": " << preserveMsg << std::endl;
    if (! preserveOk)
        fail ("BroadcastChunkPreserver::preserve() reported failure");

    MemoryBlock fixedBytes; outFile.loadFileAsData (fixedBytes);
    const String fixedIxml = testwav::findChunkText (fixedBytes, "iXML");

    if (! testwav::hasChunk (fixedBytes, "iXML")) fail ("iXML chunk still absent after preservation");
    else                                          ok ("iXML chunk present after preservation");

    for (const auto& token : testwav::classicFields())
    {
        if (! testwav::ixmlFieldPresent (srcIxml, token)) continue; // not in fixture -> skip
        if (testwav::ixmlFieldPresent (fixedIxml, token)) ok ("classic iXML preserved: " + token);
        else                                              fail ("classic iXML LOST after preservation: " + token);
    }

    // The preserved file must still be a valid WAV JUCE can open with the same audio length.
    {
        std::unique_ptr<AudioFormatReader> fixedReader (wav.createReaderFor (new FileInputStream (outFile), true));
        if (fixedReader == nullptr)            fail ("preserved file no longer opens as WAV");
        else if (fixedReader->lengthInSamples != srcLen)
            fail ("preserved file audio length changed: " + String (fixedReader->lengthInSamples));
        else                                   ok ("preserved file still valid WAV, audio intact");
    }

    // bext must also still be present (we must not have clobbered it).
    if (testwav::hasChunk (fixedBytes, "bext")
        && testwav::findChunkText (fixedBytes, "bext").contains (testwav::bextDescription()))
        ok ("bext still intact after preservation");
    else
        fail ("bext damaged by preservation");

    // ---- PART C: preservation into an RF64/>4GB output (item 8) ----
    // Uses a small but spec-valid RF64 dest so the 64-bit ds64 path is exercised without a
    // 4 GB file. preserve() must append iXML and patch the ds64 riffSize (not the sentinel).
    std::cout << "\n--- PART C: preserve into an RF64 (>4GB) output ---" << std::endl;
    const auto rf64Dest = tempDir.getChildFile ("fixture_rf64.wav");
    const MemoryBlock rf64 = testwav::buildRf64NoMetaWav (2000);
    if (! rf64Dest.replaceWithData (rf64.getData(), rf64.getSize()))
        fail ("could not write RF64 fixture");

    // The crafted RF64 must be readable by JUCE up front, else the test proves nothing.
    {
        std::unique_ptr<AudioFormatReader> r (wav.createReaderFor (new FileInputStream (rf64Dest), true));
        if (r == nullptr)               fail ("crafted RF64 fixture not readable by JUCE");
        else if (r->lengthInSamples != 2000) fail ("RF64 fixture length " + String (r->lengthInSamples) + " != 2000");
        else                            ok ("crafted RF64 fixture valid (2000 mono samples)");
    }

    String rf64Msg;
    const bool rf64Ok = BroadcastChunkPreserver::preserve (srcFile, rf64Dest, rf64Msg);
    std::cout << "    preserve() -> " << (rf64Ok ? "ok" : "FALSE") << ": " << rf64Msg << std::endl;
    if (! rf64Ok) fail ("preserve() failed on RF64 output");

    MemoryBlock rf64Bytes; rf64Dest.loadFileAsData (rf64Bytes);
    const String rf64Ixml = testwav::findChunkText (rf64Bytes, "iXML");
    if (! testwav::hasChunk (rf64Bytes, "iXML")) fail ("RF64: iXML not appended");
    else                                         ok ("RF64: iXML appended past the data sentinel");
    for (const auto& token : testwav::classicFields())
        if (testwav::ixmlFieldPresent (srcIxml, token) && ! testwav::ixmlFieldPresent (rf64Ixml, token))
            fail ("RF64: classic iXML LOST: " + token);

    // ds64 riffSize must now equal (file size - 8); the bytes-4..7 sentinel must be untouched.
    {
        const uint8* d = (const uint8*) rf64Bytes.getData();
        const uint64 riffSize = testwav::readU64LE (d + 20);
        if (testwav::readU32LE (d + 4) != 0xFFFFFFFFu) fail ("RF64: bytes 4..7 sentinel was overwritten");
        if (riffSize != (uint64) rf64Bytes.getSize() - 8)
            fail ("RF64: ds64 riffSize " + String (riffSize) + " != fileSize-8 " + String (rf64Bytes.getSize() - 8));
        else ok ("RF64: ds64 riffSize patched correctly");
    }

    // Still a valid RF64 WAV with intact audio after the append.
    {
        std::unique_ptr<AudioFormatReader> r (wav.createReaderFor (new FileInputStream (rf64Dest), true));
        if (r == nullptr)                    fail ("RF64: file no longer readable after preservation");
        else if (r->lengthInSamples != 2000) fail ("RF64: audio length changed to " + String (r->lengthInSamples));
        else                                 ok ("RF64: still valid WAV, audio intact after append");
    }

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
