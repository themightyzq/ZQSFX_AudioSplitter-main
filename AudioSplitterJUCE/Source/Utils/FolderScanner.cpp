#include "FolderScanner.h"

#include <algorithm>

//==============================================================================
FolderScanner::FolderScanner(const juce::File& dir, ProgressCallback progress, DoneCallback done)
    : juce::Thread("FolderScanner"),
      folder(dir),
      onProgress(std::move(progress)),
      onDone(std::move(done))
{
    startThread();
}

FolderScanner::~FolderScanner()
{
    // No callback may run after this point, even one already queued on the message thread.
    alive->store(false);
    stopThread(10000);
}

//==============================================================================
void FolderScanner::run()
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.wav");
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    const int total = files.size();
    std::vector<Entry> entries;
    entries.reserve((size_t) total);
    int skipped = 0;
    double lastProgress = juce::Time::getMillisecondCounterHiRes();

    for (int i = 0; i < total; ++i)
    {
        if (threadShouldExit())
            return;

        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(files.getReference(i)));
        if (reader != nullptr)
            entries.push_back({ files.getReference(i), (int) reader->numChannels });
        else
            ++skipped;

        const double now = juce::Time::getMillisecondCounterHiRes();
        if (onProgress && now - lastProgress >= 100.0)
        {
            lastProgress = now;
            auto liveness = alive;
            auto callback = onProgress;
            const int scanned = i + 1;
            juce::MessageManager::callAsync([liveness, callback, scanned, total]
            {
                if (liveness->load())
                    callback(scanned, total);
            });
        }
    }

    if (threadShouldExit())
        return;

    auto liveness = alive;
    auto callback = onDone;
    juce::MessageManager::callAsync([liveness, callback, entries = std::move(entries), skipped]() mutable
    {
        if (liveness->load() && callback)
            callback(std::move(entries), skipped);
    });
}
