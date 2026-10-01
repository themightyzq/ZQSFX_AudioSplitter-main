// splitter_ui_snapshot: render the ZQ SFX Audio Splitter's main window content headlessly to a
// PNG.
//
//   splitter_ui_snapshot <out.png> [scale] [width height]
//
// scale defaults to 2.0; width/height default to MainWindow's default size
// (MainComponent::kDefaultWidth x kDefaultHeight) -- pass MainComponent::kMinWidth x kMinHeight
// (960 600) to check the minimum resize floor (the values passed to setResizeLimits()).
//
// Optional trailing arguments (after width height):
//   --file <input.wav>   load that file into the Single File tab first (shows its channel grid)
//   --batch              show the Batch tab
//
// The look-and-feel regression gate: render before a UI change, render after, compare. This is a
// STANDALONE APP, not a plugin -- there is no AudioProcessor/editor, so the tool constructs
// MainComponent directly (DePump's depump_ui_snapshot is the worked example for apps).
//
// Determinism: nothing here pumps JUCE's message loop (no runDispatchLoop), and the snapshot is
// taken immediately after construction/resize, before any Timer could fire. (--file creates the
// "<name>_split" output folder beside the input, as dropping a file in the app does.)

#include "MainComponent.h"
#include "Utils/ConfigManager.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: splitter_ui_snapshot <out.png> [scale] [width height]\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;

    const auto out = juce::File::getCurrentWorkingDirectory().getChildFile(juce::String(argv[1]));
    const float scale = argc > 2 ? juce::String(argv[2]).getFloatValue() : 2.0f;

    const int width  = argc > 4 ? juce::String(argv[3]).getIntValue() : MainComponent::kDefaultWidth;
    const int height = argc > 4 ? juce::String(argv[4]).getIntValue() : MainComponent::kDefaultHeight;

    juce::String inputFile;
    bool showBatch = false;
    for (int i = 5; i < argc; ++i)
    {
        const juce::String arg(argv[i]);
        if (arg == "--file" && i + 1 < argc)
            inputFile = argv[++i];
        else if (arg == "--batch")
            showBatch = true;
    }

    int result = 0;
    {
        // MainComponent no longer installs its own LookAndFeel (Phase 1): the house LookAndFeel
        // (zqsfx_ui, via ModernLookAndFeel) is installed as the JUCE default by
        // ScopedHouseLookAndFeel, owned by the real application ahead of its window
        // (AudioSplitterApplication.h/.cpp) and, here, by this tool -- DePump's
        // depump_ui_snapshot is the worked example for apps. Destruction order matters: `content`
        // must be torn down before `look`/`config` are (reverse declaration order), so it is
        // declared last.
        MainComponent::ScopedHouseLookAndFeel look;
        ConfigManager config; // defaults only: loadConfig()/saveConfig() are never called, so
                               // this never touches disk.
        MainComponent content(&config);
        content.setSize(width, height);

        if (inputFile.isNotEmpty())
            content.getSingleFileSplitter()->filesDropped(juce::StringArray(juce::File::getCurrentWorkingDirectory()
                                                                             .getChildFile(inputFile).getFullPathName()), 0, 0);
        if (showBatch)
            content.setCurrentTab(1);

        const auto image = content.createComponentSnapshot(content.getLocalBounds(), true, scale);
        out.getParentDirectory().createDirectory();
        out.deleteFile();
        juce::FileOutputStream stream(out);
        juce::PNGImageFormat png;
        if (!stream.openedOk() || !png.writeImageToStream(image, stream))
        {
            std::cerr << "could not write " << out.getFullPathName() << "\n";
            result = 1;
        }
        else
        {
            std::cout << out.getFullPathName() << "  " << image.getWidth() << "x" << image.getHeight() << "\n";
        }
    }
    return result;
}
