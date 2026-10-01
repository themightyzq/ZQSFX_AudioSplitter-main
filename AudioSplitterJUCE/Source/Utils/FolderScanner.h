#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

//==============================================================================
/**
 * FolderScanner - finds the readable WAV files in a folder and their channel counts on a
 * background thread, so choosing a folder with thousands of files (or one on a slow network
 * drive) never freezes the window, and starting a batch needs no file I/O on the message thread.
 *
 * Opens each file once (header only). Entries come back sorted by file name. Callbacks run on
 * the message thread; none runs after the scanner is destroyed or cancelled. Create and destroy
 * on the message thread; destroying joins the thread (it stops between files).
 */
class FolderScanner : public juce::Thread
{
public:
    struct Entry
    {
        juce::File file;
        int numChannels {0};
    };

    /** scanned = files examined so far, total = WAV files found. Throttled to about 10 per second. */
    using ProgressCallback = std::function<void(int scanned, int total)>;

    /** Called once, with every readable file and the number skipped as unreadable. */
    using DoneCallback = std::function<void(std::vector<Entry> entries, int skipped)>;

    FolderScanner(const juce::File& folder, ProgressCallback progress, DoneCallback done);
    ~FolderScanner() override;

    void run() override;

private:
    const juce::File folder;
    ProgressCallback onProgress;
    DoneCallback onDone;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FolderScanner)
};
