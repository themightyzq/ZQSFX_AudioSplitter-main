/*
    SplitSafetyTest
    ---------------
    Data safety and the claims the README makes, proven through the real AudioFileProcessor:

      1. An existing output survives a failed run (unsupported bit depth, unwritable folder,
         and a failure on the SECOND of two outputs). The old code deleted existing outputs
         before writing the new ones and never checked a write.
      2. A successful run replaces the old file with a verified one and leaves no temp files.
      3. A cancelled run reports wasCancelled, writes nothing and leaves nothing behind.
      4. Colliding output names are refused before anything is written.
      5. 32-channel files split into 32 mono files.
      6. Sample-rate override really resamples (48k -> 96k / 192k keeps pitch and duration).
      7. 32-bit output is IEEE float; 24-bit is integer PCM.
      8. Stereo-to-mono mixes a 2-channel file to one mono file; other layouts are untouched.
*/

#include <JuceHeader.h>
#include "AudioFileProcessor.h"
#include "TestWavUtil.h"
#include "BroadcastChunkPreserver.h"

#if JUCE_MAC || JUCE_LINUX
 #include <unistd.h>
#endif

using namespace juce;

namespace
{
    int failures = 0;

    void fail (const String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; }
    void ok   (const String& msg) { std::cout << "  [ ok ] " << msg << std::endl; }
    void check (bool condition, const String& msg) { if (condition) ok (msg); else fail (msg); }

    File makeWav (const File& file, int channels, double sampleRate, int bits, int frames,
                  const std::function<float (int channel, int frame)>& sample)
    {
        file.deleteFile();
        WavAudioFormat wav;
        std::unique_ptr<AudioFormatWriter> writer (wav.createWriterFor (new FileOutputStream (file), sampleRate,
                                                                        (unsigned int) channels, bits, {}, 0));
        jassert (writer != nullptr);
        AudioBuffer<float> buffer (channels, frames);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < frames; ++i)
                buffer.setSample (c, i, sample (c, i));
        writer->writeFromAudioSampleBuffer (buffer, 0, frames);
        return file;
    }

    std::unique_ptr<AudioFormatReader> openWav (const File& f)
    {
        WavAudioFormat wav;
        return std::unique_ptr<AudioFormatReader> (wav.createReaderFor (new FileInputStream (f), true));
    }

    AudioBuffer<float> readAll (const File& f, double* sampleRate = nullptr, bool* isFloat = nullptr, int* bits = nullptr)
    {
        auto r = openWav (f);
        AudioBuffer<float> out;
        if (r == nullptr) return out;
        out.setSize ((int) r->numChannels, (int) r->lengthInSamples);
        r->read (&out, 0, (int) r->lengthInSamples, 0, true, true);
        if (sampleRate != nullptr) *sampleRate = r->sampleRate;
        if (isFloat != nullptr)    *isFloat = r->usesFloatingPointData;
        if (bits != nullptr)       *bits = (int) r->bitsPerSample;
        return out;
    }

    String readBytes (const File& f) { MemoryBlock mb; f.loadFileAsData (mb); return mb.toString(); }

    AudioFileProcessor::ProcessingResult runSplit (const AudioFileProcessor::ProcessingOptions& options)
    {
        AudioFileProcessor processor;
        processor.startProcessing (options);
        return processor.waitForResult (60000);
    }

    // Files in a folder, ignoring nothing: leftover temporaries must show up here.
    StringArray listNames (const File& dir)
    {
        StringArray names;
        for (const auto& f : dir.findChildFiles (File::findFiles, false))
            names.add (f.getFileName());
        names.sort (false);
        return names;
    }

    AudioFileProcessor::ProcessingOptions baseOptions (const File& src, const File& outDir, std::vector<int> channels)
    {
        AudioFileProcessor::ProcessingOptions o;
        o.inputFilePath = src.getFullPathName();
        o.outputDirectory = outDir.getFullPathName();
        o.selectedChannels = std::move (channels);
        o.preserveMetadata = true;
        return o;
    }

    //==========================================================================
    void testExistingOutputSurvivesBadBitDepth (const File& root)
    {
        std::cout << "-- existing output survives an unsupported bit depth" << std::endl;
        const auto src = root.getChildFile ("src_bitdepth.wav");
        const auto out = root.getChildFile ("out_bitdepth");
        out.createDirectory();
        const MemoryBlock fixture = testwav::buildFixtureWav (2000);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        const auto old1 = out.getChildFile ("src_bitdepth_chan1.wav");
        const auto old2 = out.getChildFile ("src_bitdepth_chan2.wav");
        old1.replaceWithText ("OLD-ONE-DO-NOT-LOSE");
        old2.replaceWithText ("OLD-TWO-DO-NOT-LOSE");

        auto options = baseOptions (src, out, { 0, 1 });
        options.bitDepth = 12; // not a WAV depth: the writer cannot be created
        const auto result = runSplit (options);

        check (! result.success, "run with an unsupported bit depth fails");
        check (result.errorMessage.isNotEmpty(), "the failure is reported: \"" + result.errorMessage + "\"");
        check (old1.existsAsFile() && readBytes (old1) == "OLD-ONE-DO-NOT-LOSE", "existing chan1 file is byte-identical");
        check (old2.existsAsFile() && readBytes (old2) == "OLD-TWO-DO-NOT-LOSE", "existing chan2 file is byte-identical");
        check (listNames (out).size() == 2, "no temporary or partial files left (" + listNames (out).joinIntoString (", ") + ")");
    }

    void testExistingOutputSurvivesReadOnlyFolder (const File& root)
    {
        std::cout << "-- existing output survives a read-only output folder" << std::endl;
        const auto src = root.getChildFile ("src_readonly.wav");
        const auto out = root.getChildFile ("out_readonly");
        out.createDirectory();
        const MemoryBlock fixture = testwav::buildFixtureWav (2000);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        const auto old1 = out.getChildFile ("src_readonly_chan1.wav");
        old1.replaceWithText ("OLD-READONLY-CONTENT");
        old1.setReadOnly (true); // also protects the file itself, as a user's locked file would be

        out.setReadOnly (true, false);

        // Some platforms/accounts can still write into a "read-only" folder (e.g. running as
        // root). The scenario only exists where the folder really refuses new files.
        const auto probe = out.getChildFile ("probe.tmp");
        const bool folderIsWritable = probe.create().wasOk();
        probe.deleteFile();

        if (folderIsWritable)
        {
            out.setReadOnly (false, false);
            std::cout << "  [skip] folder stayed writable on this account; read-only case not applicable" << std::endl;
            return;
        }

        auto options = baseOptions (src, out, { 0, 1 });
        const auto result = runSplit (options);

        out.setReadOnly (false, false);
        old1.setReadOnly (false);

        check (! result.success, "run into a read-only folder fails");
        check (result.errorMessage.isNotEmpty(), "the failure is reported: \"" + result.errorMessage + "\"");
        check (old1.existsAsFile() && readBytes (old1) == "OLD-READONLY-CONTENT", "existing file is byte-identical");
        check (listNames (out).size() == 1, "nothing else was created (" + listNames (out).joinIntoString (", ") + ")");
    }

    void testSecondOutputFailureLeavesFirstUntouched (const File& root)
    {
        std::cout << "-- a failure on the second output leaves the first existing file untouched" << std::endl;
        const auto src = root.getChildFile ("src_second.wav");
        const auto out = root.getChildFile ("out_second");
        out.createDirectory();
        const MemoryBlock fixture = testwav::buildFixtureWav (2000);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        const auto old1 = out.getChildFile ("src_second_keep.wav");
        old1.replaceWithText ("OLD-KEEP-CONTENT");

        auto options = baseOptions (src, out, { 0, 1 });
        // Channel 2's file would live in a folder that does not exist, so it cannot be created
        // after channel 1 has already been written successfully.
        options.customChannelNames = "keep,no_such_folder/two";
        const auto result = runSplit (options);

        check (! result.success, "run fails when the second output cannot be created");
        check (result.errorMessage.isNotEmpty(), "the failure is reported: \"" + result.errorMessage + "\"");
        check (old1.existsAsFile() && readBytes (old1) == "OLD-KEEP-CONTENT",
               "the first output's existing file is still the old file");
        check (listNames (out).size() == 1, "no temporary files left (" + listNames (out).joinIntoString (", ") + ")");
    }

    void testSuccessReplacesAndLeavesNoTemps (const File& root)
    {
        std::cout << "-- a successful run replaces the old file with a verified one" << std::endl;
        const auto src = root.getChildFile ("src_replace.wav");
        const auto out = root.getChildFile ("out_replace");
        out.createDirectory();
        const MemoryBlock fixture = testwav::buildFixtureWav (2000);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        const auto old1 = out.getChildFile ("src_replace_chan1.wav");
        old1.replaceWithText ("OLD");

        const auto result = runSplit (baseOptions (src, out, { 0, 1 }));

        check (result.success, "run succeeds: " + result.errorMessage);
        check (result.outputFiles.size() == 2, "two outputs reported");
        check (listNames (out).size() == 2, "exactly two files, no temporaries (" + listNames (out).joinIntoString (", ") + ")");

        auto r = openWav (old1);
        check (r != nullptr && r->numChannels == 1 && r->lengthInSamples == 2000, "chan1 was replaced by a valid 2000-sample mono WAV");
    }

    void testCancel (const File& root)
    {
        std::cout << "-- cancel writes nothing and says so" << std::endl;
        const auto src = root.getChildFile ("src_cancel.wav");
        const auto out = root.getChildFile ("out_cancel");
        out.createDirectory();
        makeWav (src, 2, 48000.0, 16, 48000 * 120, [] (int c, int i) { return 0.25f * (float) std::sin (0.01 * i + c); });

        const auto old1 = out.getChildFile ("src_cancel_chan1.wav");
        old1.replaceWithText ("OLD-CANCEL-CONTENT");

        AudioFileProcessor processor;
        processor.startProcessing (baseOptions (src, out, { 0, 1 }));
        processor.requestCancel();
        const auto result = processor.waitForResult (60000);

        check (! result.success, "cancelled run is not a success");
        check (result.wasCancelled, "result.wasCancelled is set");
        check (result.outputFiles.isEmpty(), "no output files reported");
        check (old1.existsAsFile() && readBytes (old1) == "OLD-CANCEL-CONTENT", "existing file untouched by the cancelled run");
        check (listNames (out).size() == 1, "no partial or temporary files (" + listNames (out).joinIntoString (", ") + ")");
    }

    void testCollisionRefused (const File& root)
    {
        std::cout << "-- colliding output names are refused" << std::endl;
        const auto src = root.getChildFile ("src_collide.wav");
        const auto out = root.getChildFile ("out_collide");
        out.createDirectory();
        const MemoryBlock fixture = testwav::buildFixtureWav (500);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        auto options = baseOptions (src, out, { 0, 1 });
        options.customChannelNames = "Same,same"; // differ only by case: one file on a default macOS/Windows volume
        const auto result = runSplit (options);

        check (! result.success && result.errorMessage.isNotEmpty(), "run is refused with a message: \"" + result.errorMessage + "\"");
        check (listNames (out).isEmpty(), "nothing was written");
    }

    void test32Channels (const File& root)
    {
        std::cout << "-- a 32-channel file splits into 32 mono files" << std::endl;
        const auto src = root.getChildFile ("src_32ch.wav");
        const auto out = root.getChildFile ("out_32ch");
        out.createDirectory();
        makeWav (src, 32, 48000.0, 24, 1000, [] (int c, int) { return (float) (c + 1) / 64.0f; });

        std::vector<int> all;
        for (int c = 0; c < 32; ++c) all.push_back (c);
        const auto result = runSplit (baseOptions (src, out, all));

        check (result.success, "32-channel run succeeds: " + result.errorMessage);
        check (result.outputFiles.size() == 32, "32 outputs reported (" + String (result.outputFiles.size()) + ")");

        int bad = 0;
        for (int c = 0; c < 32; ++c)
        {
            const auto f = out.getChildFile ("src_32ch_chan" + String (c + 1) + ".wav");
            const auto buf = readAll (f);
            if (buf.getNumChannels() != 1 || buf.getNumSamples() != 1000
                || std::abs (buf.getSample (0, 500) - (float) (c + 1) / 64.0f) > 1.0e-4f)
                ++bad;
        }
        check (bad == 0, "every output holds exactly its own source channel (" + String (bad) + " wrong)");
    }

    // Zero crossings of the segment [from, to) of channel 0.
    int zeroCrossings (const AudioBuffer<float>& b, int from, int to)
    {
        int n = 0;
        for (int i = from + 1; i < to; ++i)
            if ((b.getSample (0, i - 1) < 0.0f) != (b.getSample (0, i) < 0.0f)) ++n;
        return n;
    }

    void testResample (const File& root, double inRate, double outRate)
    {
        std::cout << "-- resample " << inRate << " -> " << outRate << std::endl;
        const auto src = root.getChildFile ("src_rs_" + String ((int) inRate) + ".wav");
        const auto out = root.getChildFile ("out_rs_" + String ((int) outRate));
        out.createDirectory();

        const int seconds = 2;
        const double hz = 1000.0;
        makeWav (src, 2, inRate, 24, (int) inRate * seconds,
                 [=] (int, int i) { return 0.5f * (float) std::sin (2.0 * MathConstants<double>::pi * hz * i / inRate); });

        auto options = baseOptions (src, out, { 0 });
        options.sampleRate = outRate;
        const auto result = runSplit (options);
        check (result.success, "resample run succeeds: " + result.errorMessage);

        double rate = 0;
        const auto buf = readAll (out.getChildFile ("src_rs_" + String ((int) inRate) + "_chan1.wav"), &rate);
        const int expected = (int) std::llround (inRate * seconds * outRate / inRate);
        check (std::abs (rate - outRate) < 0.5, "output file is " + String (rate, 0) + " Hz");
        check (buf.getNumSamples() == expected, "duration preserved: " + String (buf.getNumSamples()) + " samples, expected " + String (expected));

        // Pitch: 1 kHz over the middle second is ~2000 zero crossings whatever the rate.
        const int mid = (int) (outRate * 0.5);
        const int crossings = zeroCrossings (buf, mid, mid + (int) outRate);
        check (std::abs (crossings - 2000) <= 20, "pitch preserved: " + String (crossings) + " zero crossings in 1 s (expect ~2000)");

        // Level: not silence and not clipped (0.5 amplitude sine => ~0.354 RMS).
        double sum = 0.0;
        for (int i = mid; i < mid + (int) outRate; ++i) sum += (double) buf.getSample (0, i) * buf.getSample (0, i);
        const double rms = std::sqrt (sum / outRate);
        check (std::abs (rms - 0.3536) < 0.02, "level preserved: RMS " + String (rms, 3) + " (expect ~0.354)");
    }


    //==========================================================================
    // Amplitude of the component at `hz` in channel 0 over [from, from+count), Hann-windowed so a
    // strong neighbouring tone does not leak into the measurement.
    double toneAmplitude (const AudioBuffer<float>& b, double sampleRate, double hz, int from, int count)
    {
        double re = 0.0, im = 0.0, windowSum = 0.0;
        for (int i = 0; i < count; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (2.0 * MathConstants<double>::pi * i / (count - 1));
            const double phase = 2.0 * MathConstants<double>::pi * hz * (from + i) / sampleRate;
            const double x = b.getSample (0, from + i) * w;
            re += x * std::cos (phase);
            im -= x * std::sin (phase);
            windowSum += w;
        }
        return 2.0 * std::sqrt (re * re + im * im) / windowSum;
    }

    // Converts a 0.5-amplitude tone from inRate to outRate and returns its measured level, in dB
    // relative to 0.5, at each frequency in `measureAt`.
    std::vector<double> convertTone (const File& root, double inRate, double outRate, double toneHz,
                                     const std::vector<double>& measureAt, const String& tag)
    {
        const auto src = root.getChildFile ("src_tone_" + tag + ".wav");
        const auto out = root.getChildFile ("out_tone_" + tag);
        out.createDirectory();
        const int seconds = 2;
        makeWav (src, 1, inRate, 24, (int) inRate * seconds,
                 [=] (int, int i) { return 0.5f * (float) std::sin (2.0 * MathConstants<double>::pi * toneHz * i / inRate); });

        auto options = baseOptions (src, out, { 0 });
        options.sampleRate = outRate;
        const auto result = runSplit (options);
        if (! result.success)
            fail ("tone conversion " + tag + " failed: " + result.errorMessage);

        const auto buf = readAll (out.getChildFile ("src_tone_" + tag + "_chan1.wav"));
        const int from = (int) (outRate * 0.5), count = (int) outRate; // the middle second, clear of the filter's edges
        std::vector<double> levels;
        for (double hz : measureAt)
            levels.push_back (20.0 * std::log10 (juce::jmax (1.0e-9, toneAmplitude (buf, outRate, hz, from, count) / 0.5)));
        return levels;
    }

    void testAntiAliasing (const File& root)
    {
        std::cout << "-- sample-rate conversion filters out what the new rate cannot hold" << std::endl;

        // 96k -> 44.1k. A 40 kHz tone is above the new Nyquist (22.05 kHz) and would fold down to
        // 44.1 - 40 = 4.1 kHz; 26 kHz would fold to 18.1 kHz. Both must be removed, not aliased.
        {
            const auto l = convertTone (root, 96000.0, 44100.0, 40000.0, { 4100.0 }, "d40");
            check (l[0] < -60.0, "96k->44.1k: 40 kHz tone aliases to 4.1 kHz at " + String (l[0], 1) + " dB (needs < -60)");
        }
        {
            const auto l = convertTone (root, 96000.0, 44100.0, 26000.0, { 18100.0 }, "d26");
            check (l[0] < -60.0, "96k->44.1k: 26 kHz tone aliases to 18.1 kHz at " + String (l[0], 1) + " dB (needs < -60)");
        }
        {
            // 20 kHz is below the new Nyquist: it must survive.
            const auto l = convertTone (root, 96000.0, 44100.0, 20000.0, { 20000.0 }, "d20");
            check (l[0] > -1.0 && l[0] < 0.5, "96k->44.1k: 20 kHz tone passes at " + String (l[0], 2) + " dB (needs within -1 .. +0.5)");
        }
        {
            const auto l = convertTone (root, 96000.0, 44100.0, 1000.0, { 1000.0 }, "d1");
            check (std::abs (l[0]) < 0.1, "96k->44.1k: 1 kHz tone is flat at " + String (l[0], 3) + " dB");
        }
        // 48k -> 96k. A 20 kHz tone's mirror image lands at 48 - 20 = 28 kHz in the new, wider band.
        {
            const auto l = convertTone (root, 48000.0, 96000.0, 20000.0, { 28000.0, 20000.0 }, "u20");
            check (l[0] < -60.0, "48k->96k: image of a 20 kHz tone at 28 kHz is " + String (l[0], 1) + " dB (needs < -60)");
            check (l[1] > -1.0 && l[1] < 0.5, "48k->96k: the 20 kHz tone itself passes at " + String (l[1], 2) + " dB");
        }
        // Non-integer ratio, 48k -> 44.1k: 23 kHz folds to 21.1 kHz.
        {
            const auto l = convertTone (root, 48000.0, 44100.0, 23500.0, { 20600.0 }, "d23");
            check (l[0] < -60.0, "48k->44.1k: 23.5 kHz tone aliases to 20.6 kHz at " + String (l[0], 1) + " dB (needs < -60)");
        }
    }

    void testBitDepthFormats (const File& root)
    {
        std::cout << "-- bit depth: 32 is IEEE float, 24 is integer PCM" << std::endl;
        const auto src = root.getChildFile ("src_bits.wav");
        makeWav (src, 2, 48000.0, 16, 4800, [] (int c, int i) { return (c == 0 ? 0.4f : -0.4f) * (float) std::sin (0.05 * i); });

        for (int depth : { 8, 16, 24, 32 })
        {
            const auto out = root.getChildFile ("out_bits_" + String (depth));
            out.createDirectory();
            auto options = baseOptions (src, out, { 0 });
            options.bitDepth = depth;
            const auto result = runSplit (options);
            check (result.success, String (depth) + "-bit run succeeds: " + result.errorMessage);

            bool isFloat = false; int bits = 0;
            const auto buf = readAll (out.getChildFile ("src_bits_chan1.wav"), nullptr, &isFloat, &bits);
            check (bits == depth, "file is " + String (bits) + "-bit");
            check (isFloat == (depth == 32), depth == 32 ? "32-bit output is IEEE float" : String (depth) + "-bit output is integer PCM");

            // Content matches the source within the target depth's quantisation.
            const float tolerance = depth == 8 ? 1.2e-2f : (depth == 16 ? 2.0e-4f : 1.0e-4f);
            check (std::abs (buf.getSample (0, 1000) - 0.4f * (float) std::sin (0.05 * 1000)) < tolerance, "sample values preserved at " + String (depth) + " bit");
        }
    }


    //==========================================================================
    String tagValue (const String& text, const String& tag)
    {
        const int s = text.indexOf ("<" + tag + ">");
        if (s < 0) return "(missing)";
        const int from = s + tag.length() + 2;
        return text.substring (from, text.indexOf (from, "</" + tag + ">"));
    }

    const char* kRateIXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?><BWFXML><IXML_VERSION>1.5</IXML_VERSION>"
        "<PROJECT>Rate Project</PROJECT><SCENE>SC09</SCENE><TAKE>004</TAKE>"
        "<SPEED><TIMECODE_RATE>25/1</TIMECODE_RATE><FILE_SAMPLE_RATE>48000</FILE_SAMPLE_RATE>"
        "<DIGITIZER_SAMPLE_RATE>48000</DIGITIZER_SAMPLE_RATE><AUDIO_BIT_DEPTH>16</AUDIO_BIT_DEPTH>"
        "<TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_HI>0</TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_HI>"
        "<TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_LO>3000000000</TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_LO>"
        "<TIMESTAMP_SAMPLE_RATE>48000</TIMESTAMP_SAMPLE_RATE></SPEED></BWFXML>";

    void testRateMetadata (const File& root)
    {
        std::cout << "-- bext/iXML rate fields describe the output after a conversion" << std::endl;
        const auto src = root.getChildFile ("src_ratemeta.wav");
        const String sourceHistory = "A=PCM,F=48000,W=16,M=stereo,T=Recorder\r\n";
        const MemoryBlock fixture = testwav::buildFixtureWav (4800, kRateIXml, sourceHistory, 123456);
        src.replaceWithData (fixture.getData(), fixture.getSize());

        {
            const auto out = root.getChildFile ("out_ratemeta");
            out.createDirectory();
            auto options = baseOptions (src, out, { 0 });
            options.sampleRate = 96000.0;
            options.bitDepth = 24;
            const auto result = runSplit (options);
            check (result.success, "48k/16-bit -> 96k/24-bit run succeeds: " + result.errorMessage);

            const auto file = out.getChildFile ("src_ratemeta_chan1.wav");
            MemoryBlock bytes; file.loadFileAsData (bytes);
            const String ixml = testwav::findChunkText (bytes, "iXML");
            check (tagValue (ixml, "FILE_SAMPLE_RATE") == "96000", "iXML FILE_SAMPLE_RATE is 96000 (" + tagValue (ixml, "FILE_SAMPLE_RATE") + ")");
            check (tagValue (ixml, "TIMESTAMP_SAMPLE_RATE") == "96000", "iXML TIMESTAMP_SAMPLE_RATE is 96000");
            check (tagValue (ixml, "AUDIO_BIT_DEPTH") == "24", "iXML AUDIO_BIT_DEPTH is 24");
            check (tagValue (ixml, "DIGITIZER_SAMPLE_RATE") == "48000", "iXML DIGITIZER_SAMPLE_RATE (the capture rate) is untouched");
            // 3,000,000,000 samples at 48k = 6,000,000,000 at 96k = 1 * 2^32 + 1,705,032,704
            check (tagValue (ixml, "TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_HI") == "1" && tagValue (ixml, "TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_LO") == "1705032704",
                   "iXML timestamp rescaled to 96k with carry into HI (HI " + tagValue (ixml, "TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_HI")
                   + ", LO " + tagValue (ixml, "TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_LO") + ")");
            check (tagValue (ixml, "SCENE") == "SC09" && tagValue (ixml, "TAKE") == "004" && tagValue (ixml, "TIMECODE_RATE") == "25/1",
                   "scene, take and timecode rate are untouched");

            auto reader = openWav (file);
            const String timeRef = reader != nullptr ? reader->metadataValues[WavAudioFormat::bwavTimeReference] : String();
            check (timeRef == "246912", "bext TimeReference doubled for 96k (" + timeRef + ")");
            const String history = reader != nullptr ? reader->metadataValues[WavAudioFormat::bwavCodingHistory] : String();
            check (history.startsWith (sourceHistory), "the recorder's own coding history is kept");
            check (history.contains ("F=96000") && history.contains ("W=24") && history.contains ("converted from 48000 Hz"),
                   "a coding-history line records the output format and the conversion");
        }
        {
            // No conversion: everything is copied verbatim.
            const auto out = root.getChildFile ("out_ratemeta_same");
            out.createDirectory();
            const auto result = runSplit (baseOptions (src, out, { 0 }));
            check (result.success, "unconverted run succeeds");
            MemoryBlock bytes; out.getChildFile ("src_ratemeta_chan1.wav").loadFileAsData (bytes);
            check (testwav::findChunkText (bytes, "iXML") == String (kRateIXml), "iXML is byte-identical to the source when nothing was converted");
            auto reader = openWav (out.getChildFile ("src_ratemeta_chan1.wav"));
            check (reader != nullptr && reader->metadataValues[WavAudioFormat::bwavTimeReference] == "123456"
                   && reader->metadataValues[WavAudioFormat::bwavCodingHistory] == sourceHistory,
                   "bext TimeReference and CodingHistory are unchanged");
        }
        {
            // Non-integer ratio, and the cue/loop/sample-period fields JUCE exposes.
            StringPairArray values;
            values.set (WavAudioFormat::bwavTimeReference, "480000");
            values.set ("NumCuePoints", "1");  values.set ("Cue0Offset", "48000");
            values.set ("NumSampleLoops", "1"); values.set ("Loop0Start", "1000"); values.set ("Loop0End", "9600");
            values.set ("SamplePeriod", "20833");
            BroadcastChunkPreserver::retargetMetadataValues (values, 48000.0, 44100.0, 16);
            check (values["bwav time reference"] == "441000" && values["Cue0Offset"] == "44100"
                   && values["Loop0Start"] == "919" && values["Loop0End"] == "8820" && values["SamplePeriod"] == "22676",
                   "TimeReference, cue offset, loop points and sample period rescale to 44.1k (" + values["bwav time reference"] + ", "
                   + values["Cue0Offset"] + ", " + values["Loop0Start"] + ", " + values["Loop0End"] + ", " + values["SamplePeriod"] + ")");
        }
    }

    void testStereoToMono (const File& root)
    {
        std::cout << "-- stereo to mono" << std::endl;
        {
            const auto src = root.getChildFile ("src_s2m.wav");
            const auto out = root.getChildFile ("out_s2m");
            out.createDirectory();
            // L = +0.5 constant, R = -0.5 constant: a perfect cancel when mixed.
            makeWav (src, 2, 48000.0, 24, 2000, [] (int c, int) { return c == 0 ? 0.5f : -0.5f; });
            auto options = baseOptions (src, out, { 0, 1 });
            options.stereoToMono = true;
            const auto result = runSplit (options);
            check (result.success && result.outputFiles.size() == 1, "stereo file + option gives ONE output file (" + String (result.outputFiles.size()) + ")");
            const auto mono = out.getChildFile ("src_s2m_mono.wav");
            const auto buf = readAll (mono);
            check (buf.getNumChannels() == 1 && buf.getNumSamples() == 2000, "output is a 2000-sample mono file");
            check (std::abs (buf.getSample (0, 1000)) < 1.0e-4f, "L +0.5 and R -0.5 mix to silence");
        }
        {
            const auto src = root.getChildFile ("src_s2m_b.wav");
            const auto out = root.getChildFile ("out_s2m_b");
            out.createDirectory();
            makeWav (src, 2, 48000.0, 24, 2000, [] (int c, int) { return c == 0 ? 0.5f : 0.3f; });
            auto options = baseOptions (src, out, { 0, 1 });
            options.stereoToMono = true;
            const auto result = runSplit (options);
            const auto buf = readAll (out.getChildFile ("src_s2m_b_mono.wav"));
            check (result.success && buf.getNumSamples() == 2000 && std::abs (buf.getSample (0, 10) - 0.4f) < 1.0e-4f,
                   "L 0.5 and R 0.3 mix to 0.4 (equal-gain average, never clips)");
        }
        {
            // Not a stereo pair: only channel 0 selected -> normal single-channel split.
            const auto src = root.getChildFile ("src_s2m_c.wav");
            const auto out = root.getChildFile ("out_s2m_c");
            out.createDirectory();
            makeWav (src, 2, 48000.0, 24, 500, [] (int c, int) { return c == 0 ? 0.5f : 0.3f; });
            auto options = baseOptions (src, out, { 0 });
            options.stereoToMono = true;
            const auto result = runSplit (options);
            check (result.success && out.getChildFile ("src_s2m_c_chan1.wav").existsAsFile() && listNames (out).size() == 1,
                   "only one channel selected: normal split, no mix");
        }
        {
            // 3 channels: the option does not apply.
            const auto src = root.getChildFile ("src_s2m_d.wav");
            const auto out = root.getChildFile ("out_s2m_d");
            out.createDirectory();
            makeWav (src, 3, 48000.0, 24, 500, [] (int c, int) { return 0.1f * (float) (c + 1); });
            auto options = baseOptions (src, out, { 0, 1, 2 });
            options.stereoToMono = true;
            const auto result = runSplit (options);
            check (result.success && listNames (out).size() == 3, "3-channel file: option has no effect, three mono files");
        }
        {
            // UCS naming of the mix.
            const auto src = root.getChildFile ("src_s2m_e.wav");
            const auto out = root.getChildFile ("out_s2m_e");
            out.createDirectory();
            makeWav (src, 2, 48000.0, 24, 500, [] (int, int) { return 0.2f; });
            auto options = baseOptions (src, out, { 0, 1 });
            options.stereoToMono = true;
            options.useUCSNaming = true;
            options.ucsCategory = "AMBNat";
            options.ucsSubcategory = "Forest";
            options.ucsDescription = "Birds";
            options.ucsChannelSuffixes = { "L", "R" };
            const auto result = runSplit (options);
            check (result.success && out.getChildFile ("AMBNat_Forest_Birds_M.wav").existsAsFile(), "UCS name of the mix ends _M");
        }
    }
}

int main()
{
    ScopedJuceInitialiser_GUI juceInit;

    std::cout << "=== ZQ SFX SplitSafetyTest ===" << std::endl;

    const auto root = File::getSpecialLocation (File::tempDirectory).getChildFile ("zqsfx_split_safety_test");
    root.deleteRecursively();
    root.createDirectory();

    testExistingOutputSurvivesBadBitDepth (root);
    testExistingOutputSurvivesReadOnlyFolder (root);
    testSecondOutputFailureLeavesFirstUntouched (root);
    testSuccessReplacesAndLeavesNoTemps (root);
    testCancel (root);
    testCollisionRefused (root);
    test32Channels (root);
    testResample (root, 48000.0, 96000.0);
    testResample (root, 48000.0, 192000.0);
    testResample (root, 96000.0, 48000.0);
    testResample (root, 44100.0, 48000.0);   // non-integer ratio (160/147), the common real-world case
    testAntiAliasing (root);
    testBitDepthFormats (root);
    testStereoToMono (root);
    testRateMetadata (root);

    root.deleteRecursively();

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
