/*
    SplitStateTest
    --------------
    Drives the real MainComponent through a whole run on both tabs, headless (messages are
    captured through UserNotice instead of opening alert windows), and checks the window state:

      - Split is disabled from the moment a run starts and stays disabled until that run's
        completion has been reported (the old code re-enabled it after one second);
      - the Cancel button shows while a run is going, is wired to stop it (it used to do
        nothing: onCancelRequested was never assigned), and is acknowledged at once;
      - a cancelled batch and a cancelled single-file split both complete, say they were
        cancelled, leave no partial or temporary files, and hand Split back;
      - after any run the progress panel is back to its idle state.
*/

#include "MainComponent.h"
#include "Utils/ConfigManager.h"
#include "Utils/UserNotice.h"

#include <iostream>

namespace
{
    int failures = 0;
    void fail (const juce::String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; }
    void ok   (const juce::String& msg) { std::cout << "  [ ok ] " << msg << std::endl; }
    void check (bool condition, const juce::String& msg) { if (condition) ok (msg); else fail (msg); }

    struct Notice { juce::String title, message; };
    std::vector<Notice> notices;

    void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

    template <typename Pred>
    bool pumpUntil (Pred done, int timeoutMs)
    {
        const double end = juce::Time::getMillisecondCounterHiRes() + timeoutMs;
        while (! done() && juce::Time::getMillisecondCounterHiRes() < end)
            pump (5);
        return done();
    }

    void makeWav (const juce::File& file, int channels, int seconds)
    {
        file.deleteFile(); // FileOutputStream appends to an existing file
        juce::WavAudioFormat wav;
        const int frames = 48000 * seconds;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), 48000.0,
                                                                              (unsigned int) channels, 16, {}, 0));
        juce::AudioBuffer<float> buffer (channels, frames);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < frames; ++i)
                buffer.setSample (c, i, 0.2f * (float) std::sin (0.02 * i + c));
        writer->writeFromAudioSampleBuffer (buffer, 0, frames);
    }

    juce::TextButton* findButton (juce::Component& root, const juce::String& text)
    {
        for (auto* child : root.getChildren())
        {
            if (auto* b = dynamic_cast<juce::TextButton*> (child))
                if (b->getButtonText() == text)
                    return b;
            if (auto* deeper = findButton (*child, text))
                return deeper;
        }
        return nullptr;
    }

    juce::StringArray names (const juce::File& dir)
    {
        juce::StringArray n;
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false)) n.add (f.getFileName());
        return n;
    }

    bool allOutputsComplete (const juce::File& dir, juce::int64 expectedSamples)
    {
        juce::WavAudioFormat wav;
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false))
        {
            std::unique_ptr<juce::AudioFormatReader> r (wav.createReaderFor (new juce::FileInputStream (f), true));
            if (r == nullptr || r->numChannels != 1 || r->lengthInSamples != expectedSamples || f.getFileName().contains ("_temp"))
                return false;
        }
        return true;
    }

    bool haveNotice (const juce::String& title) { for (auto& n : notices) if (n.title == title) return true; return false; }
    juce::String noticeMessage (const juce::String& title) { for (auto& n : notices) if (n.title == title) return n.message; return {}; }

    void startWithKeyboard (MainComponent& main)
    {
        main.keyPressed (juce::KeyPress (juce::KeyPress::returnKey, juce::ModifierKeys::commandModifier, 0));
    }

    //==========================================================================
    void batchRun (const juce::File& root, bool cancel)
    {
        const juce::String label = cancel ? "batch, cancelled" : "batch, left to finish";
        std::cout << "-- " << label << std::endl;
        notices.clear();

        const auto in = root.getChildFile (cancel ? "in_cancel" : "in_run");  in.createDirectory();
        const auto out = root.getChildFile (cancel ? "out_cancel" : "out_run"); out.createDirectory();
        const int files = 24;
        for (int i = 0; i < files; ++i)
            makeWav (in.getChildFile ("f" + juce::String (i) + ".wav"), 2, 3);

        ConfigManager config;
        config.setLastInputDir (in.getFullPathName());
        config.setLastOutputDir (out.getFullPathName());

        MainComponent main (&config);
        main.setSize (MainComponent::kDefaultWidth, MainComponent::kDefaultHeight);
        main.setCurrentTab (1);

        // The input folder is read on a background thread: constructing the window returned with
        // the scan still running, and Split waits for it.
        check (main.getBatchSplitter()->isScanning(), "the input folder is being scanned in the background");
        check (! main.isSplitEnabled(), "Split waits while the folder is scanned");
        check (pumpUntil ([&] { return ! main.getBatchSplitter()->isScanning(); }, 30000), "the scan finished");

        check (main.isSplitEnabled(), "Split is enabled with an input and an output folder chosen");
        auto* cancelButton = findButton (*main.getProgressPanel(), "Cancel");
        check (cancelButton != nullptr && ! cancelButton->isVisible(), "Cancel is hidden while idle");

        startWithKeyboard (main);
        check (! main.isSplitEnabled(), "Split is disabled the moment the run starts");
        check (cancelButton != nullptr && cancelButton->isVisible() && cancelButton->isEnabled(), "Cancel is showing and enabled during the run");

        if (cancel)
        {
            cancelButton->onClick(); // the button's own handler, through ProgressPanel::onCancelRequested
            check (! cancelButton->isEnabled(), "Cancel is acknowledged at once (disabled so it cannot be pressed twice)");
        }

        // Until the run's completion has been reported, Split must never come back.
        bool enabledEarly = false;
        const bool completed = pumpUntil ([&]
        {
            if (! haveNotice ("Batch Processing Complete") && main.isSplitEnabled())
                enabledEarly = true;
            return haveNotice ("Batch Processing Complete");
        }, 60000);

        check (completed, "the run reported its completion");
        check (! enabledEarly, "Split stayed disabled until the completion was reported");

        const auto message = noticeMessage ("Batch Processing Complete");
        if (cancel)
            check (message.contains ("cancelled"), "the completion message says the batch was cancelled");
        else
            check (message.contains ("Successful: " + juce::String (files)), "the completion message reports all " + juce::String (files) + " files successful");

        pump (50);
        check (main.isSplitEnabled(), "Split is enabled again after the run");
        check (cancelButton != nullptr && ! cancelButton->isVisible(), "Cancel is hidden again after the run");
        check (allOutputsComplete (out, 48000 * 3), "every file in the output folder is complete (" + juce::String (out.getNumberOfChildFiles (juce::File::findFiles)) + " files, no temporaries)");

        if (! cancel)
            check (out.getNumberOfChildFiles (juce::File::findFiles) == files * 2, "all " + juce::String (files * 2) + " outputs written");

        // The next run works (Split really is usable again, not just enabled).
        notices.clear();
        startWithKeyboard (main);
        check (! main.isSplitEnabled(), "a second run starts and disables Split again");
        if (auto* b = findButton (*main.getProgressPanel(), "Cancel")) b->onClick();
        pumpUntil ([&] { return haveNotice ("Batch Processing Complete"); }, 60000);
        pump (50);
        check (main.isSplitEnabled(), "Split is enabled after the second run too");
    }


    void startReadsNoHeaders (const juce::File& root)
    {
        std::cout << "-- starting a batch reads no file headers on the message thread" << std::endl;
        notices.clear();

        const auto in = root.getChildFile ("in_noreread");  in.createDirectory();
        const auto out = root.getChildFile ("out_noreread"); out.createDirectory();
        for (int i = 0; i < 3; ++i)
            makeWav (in.getChildFile ("g" + juce::String (i) + ".wav"), 2, 1);

        ConfigManager config;
        config.setLastInputDir (in.getFullPathName());
        config.setLastOutputDir (out.getFullPathName());

        MainComponent main (&config);
        main.setSize (MainComponent::kDefaultWidth, MainComponent::kDefaultHeight);
        main.setCurrentTab (1);
        pumpUntil ([&] { return ! main.getBatchSplitter()->isScanning(); }, 30000);
        check (main.getBatchSplitter()->getInputDirectory() == in.getFullPathName(), "scan finished for the input folder");

        // After the scan, g1.wav becomes a mono file. Its channel count was recorded by the scan
        // (2), so if starting re-read headers it would be processed as mono and succeed; using
        // the scan's count, asking for channel 2 of a mono file fails.
        makeWav (in.getChildFile ("g1.wav"), 1, 1);

        startWithKeyboard (main);
        pumpUntil ([&] { return haveNotice ("Batch Processing Complete"); }, 60000);
        const auto message = noticeMessage ("Batch Processing Complete");
        check (message.contains ("Successful: 2") && message.contains ("Failed: 1"),
               "channel counts came from the scan, not from a header read at start (2 ok, 1 failed)");
    }

    void ucsDefaults()
    {
        std::cout << "-- UCS Subcategory is never blank" << std::endl;
        ConfigManager config;
        MainComponent main (&config);
        main.setSize (MainComponent::kDefaultWidth, MainComponent::kDefaultHeight);

        auto* panel = main.getUCSNamingPanel();
        auto* ucs = main.getUCSManager();
        const auto firstCategory = panel->getCategory();
        check (firstCategory.isNotEmpty() && panel->getSubcategory().isNotEmpty()
               && panel->getSubcategory() == ucs->getSubcategories (firstCategory)[0],
               "right after construction: category " + firstCategory + ", subcategory \"" + panel->getSubcategory() + "\"");

        panel->setCategory ("FOLExt");
        pumpUntil ([&] { return panel->getCategory() == "FOLExt"; }, 5000);
        pump (50);
        check (panel->getCategory() == "FOLExt" && panel->getSubcategory() == ucs->getSubcategories ("FOLExt")[0],
               "after choosing FOLExt the first FOLExt subcategory is selected (\"" + panel->getSubcategory() + "\")");
        check (panel->generateFilename ("L").startsWith ("FOLExt_" + ucs->getSubcategories ("FOLExt")[0].removeCharacters (" ")),
               "the preview name includes it (" + panel->generateFilename ("L") + ")");
    }

    void singleFileCancel (const juce::File& root)
    {
        std::cout << "-- single file, cancelled" << std::endl;
        notices.clear();

        const auto src = root.getChildFile ("single_src.wav");
        makeWav (src, 6, 60);

        ConfigManager config;
        MainComponent main (&config);
        main.setSize (MainComponent::kDefaultWidth, MainComponent::kDefaultHeight);
        main.getSingleFileSplitter()->filesDropped (juce::StringArray (src.getFullPathName()), 0, 0);

        check (main.isSplitEnabled(), "Split is enabled with a file loaded");
        auto* cancelButton = findButton (*main.getProgressPanel(), "Cancel");

        startWithKeyboard (main);
        check (! main.isSplitEnabled(), "Split is disabled the moment the split starts");
        check (cancelButton != nullptr && cancelButton->isVisible(), "Cancel is showing during a single-file split");

        cancelButton->onClick();
        check (! cancelButton->isEnabled(), "Cancel is acknowledged at once");

        bool enabledEarly = false;
        const bool completed = pumpUntil ([&]
        {
            const bool reported = haveNotice ("Processing Cancelled") || haveNotice ("Processing Complete") || haveNotice ("Processing Failed");
            if (! reported && main.isSplitEnabled())
                enabledEarly = true;
            return reported;
        }, 60000);

        check (completed, "the split reported its end");
        check (! enabledEarly, "Split stayed disabled until the end was reported");
        check (haveNotice ("Processing Cancelled"), "it was reported as cancelled, not as a failure");

        pump (50);
        check (main.isSplitEnabled(), "Split is enabled again");
        check (cancelButton != nullptr && ! cancelButton->isVisible(), "Cancel is hidden again");

        const auto outDir = root.getChildFile ("single_src_split");
        check (names (outDir).isEmpty(), "nothing was written to the output folder (" + names (outDir).joinIntoString (", ") + ")");
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    std::cout << "=== ZQ SFX SplitStateTest ===" << std::endl;

    UserNotice::handler() = [] (const juce::MessageBoxOptions& o)
    {
        notices.push_back ({ o.getTitle(), o.getMessage() });
    };

    const auto root = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zqsfx_split_state_test");
    root.deleteRecursively();
    root.createDirectory();

    {
        MainComponent::ScopedHouseLookAndFeel look;
        batchRun (root, false);
        batchRun (root, true);
        singleFileCancel (root);
        startReadsNoHeaders (root);
        ucsDefaults();
    }

    root.deleteRecursively();

    std::cout << "\n=== " << (failures == 0 ? "PASS" : "FAIL") << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
