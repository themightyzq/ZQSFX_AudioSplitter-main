// splitter_ui_snapshot: render the ZQ SFX Audio Splitter's main window content headlessly to a
// PNG.
//
//   splitter_ui_snapshot <out.png> [scale] [width height]
//
// scale defaults to 2.0; width/height default to MainWindow's defaultWidth/defaultHeight
// (1200x1110) -- pass 1000 850 to check the minimum resize floor (MainWindow's
// minWidth/minHeight, the values passed to setResizeLimits()).
//
// The look-and-feel regression gate: render before a UI change, render after, compare. This is a
// STANDALONE APP, not a plugin -- there is no AudioProcessor/editor, so the tool constructs
// MainComponent directly (DePump's depump_ui_snapshot is the worked example for apps).
//
// Determinism: nothing here pumps JUCE's message loop (no runDispatchLoop), and the snapshot is
// taken immediately after construction/resize, before any Timer could fire.

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

    // MainWindow.cpp: defaultWidth/defaultHeight (also its setResizeLimits() minima, 1000x850).
    constexpr int kDefaultWidth = 1200;
    constexpr int kDefaultHeight = 1110;
    const int width  = argc > 4 ? juce::String(argv[3]).getIntValue() : kDefaultWidth;
    const int height = argc > 4 ? juce::String(argv[4]).getIntValue() : kDefaultHeight;

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
