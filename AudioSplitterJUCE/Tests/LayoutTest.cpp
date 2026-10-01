/*
    LayoutTest
    ----------
    Constructs the real MainComponent at several window sizes (the minimum, a 13-inch laptop
    default, and the old 1000x850 minimum) on both tabs, with and without a 32-channel file
    loaded and with the batch progress display showing, and checks the layout geometrically:

      1. Every visible component lies inside its parent's bounds (nothing is clipped by its
         own container). The scrolling column is exempt from being inside its viewport, which
         is the point of it, but everything inside the column must fit the column.
      2. No two visible sibling controls overlap.
      3. No control sits in a group's title band (the OPTIONS title used to run into the first
         checkbox).
      4. Label and button text fits the control it is drawn in.
      5. The window minimum and default fit a 13-inch laptop.

    The old layout failed 1 and 2 at 1000x850 (the Custom Names group fell below the Options
    panel's bottom edge) and 3 at every size.
*/

#include "MainComponent.h"
#include "Utils/ConfigManager.h"

#include <iostream>

namespace
{
    int failures = 0;
    int checks = 0;

    void fail (const juce::String& msg) { std::cerr << "  [FAIL] " << msg << std::endl; ++failures; }

    juce::String describe (const juce::Component& c)
    {
        juce::String name = c.getName().isNotEmpty() ? c.getName() : juce::String ("(unnamed)");
        if (auto* b = dynamic_cast<const juce::Button*> (&c))
            name += " \"" + b->getButtonText() + "\"";
        if (auto* l = dynamic_cast<const juce::Label*> (&c))
            name += " \"" + l->getText().substring (0, 30) + "\"";
        return name + " [" + juce::String (typeid (c).name()) + "] " + c.getBounds().toString();
    }

    template <typename T>
    T* findDescendant (juce::Component& root)
    {
        for (auto* child : root.getChildren())
        {
            if (auto* hit = dynamic_cast<T*> (child))
                return hit;
            if (auto* deeper = findDescendant<T> (*child))
                return deeper;
        }
        return nullptr;
    }

    bool isOnScreen (const juce::Component& c)
    {
        return c.isVisible() && c.getWidth() > 0 && c.getHeight() > 0;
    }

    // JUCE's own composite controls lay out internal parts we do not control (a text editor's
    // scrolling text holder, the tab bar's overlapping tab buttons); only their outer bounds matter.
    bool hasOpaqueInternals (const juce::Component& c)
    {
        return dynamic_cast<const juce::TabbedButtonBar*> (&c) != nullptr
            || dynamic_cast<const juce::TextEditor*> (&c) != nullptr
            || dynamic_cast<const juce::ComboBox*> (&c) != nullptr
            || dynamic_cast<const juce::ScrollBar*> (&c) != nullptr;
    }

    void visit (juce::Component& c, const std::function<void (juce::Component&)>& f)
    {
        f (c);
        if (hasOpaqueInternals (c))
            return;

        for (auto* child : c.getChildren())
            if (isOnScreen (*child))
                visit (*child, f);
    }

    float textWidth (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox (0, -1, true).getWidth();
    }

    void checkLayout (MainComponent& main, const juce::String& scenario)
    {
        const int failuresBefore = failures;
        auto* viewport = findDescendant<juce::Viewport> (main);
        auto* column = viewport != nullptr ? viewport->getViewedComponent() : nullptr;

        visit (main, [&] (juce::Component& c)
        {
            auto* parent = c.getParentComponent();
            if (parent == nullptr)
                return;

            ++checks;

            // 1. inside the parent (the scrolling column is allowed to extend past its viewport)
            if (&c != column && ! parent->getLocalBounds().contains (c.getBounds()))
                fail (scenario + ": outside its parent " + parent->getName() + " (" + parent->getLocalBounds().toString()
                      + "): " + describe (c));

            // 4. text fits (labels and text buttons; the look-and-feel supplies the font)
            if (auto* label = dynamic_cast<juce::Label*> (&c))
            {
                if (label->getText().isNotEmpty())
                {
                    const float needed = textWidth (label->getLookAndFeel().getLabelFont (*label), label->getText());
                    if (needed > (float) label->getWidth() - label->getBorderSize().getLeftAndRight())
                        fail (scenario + ": label text is wider than its box (" + juce::String (needed, 0) + " px): " + describe (c));
                }
            }
            else if (auto* button = dynamic_cast<juce::TextButton*> (&c))
            {
                const float needed = textWidth (button->getLookAndFeel().getTextButtonFont (*button, button->getHeight()), button->getButtonText());
                if (needed > (float) button->getWidth() - 8.0f)
                    fail (scenario + ": button text is wider than the button (" + juce::String (needed, 0) + " px): " + describe (c));
            }

            // 2. + 3. sibling overlap and group title band
            for (auto* sibling : parent->getChildren())
            {
                if (sibling == &c || ! isOnScreen (*sibling) || sibling < &c)
                    continue; // each unordered pair once

                const bool cIsGroup = dynamic_cast<juce::GroupComponent*> (&c) != nullptr;
                const bool sIsGroup = dynamic_cast<juce::GroupComponent*> (sibling) != nullptr;

                if (! cIsGroup && ! sIsGroup && c.getBounds().intersects (sibling->getBounds()))
                    fail (scenario + ": overlaps sibling:\n        " + describe (c) + "\n        " + describe (*sibling));

                // A group is drawn behind controls placed on it; those must clear its title band.
                if (cIsGroup != sIsGroup)
                {
                    auto& group = cIsGroup ? c : *sibling;
                    auto& other = cIsGroup ? *sibling : c;
                    const auto band = group.getBounds().withHeight (ModernLookAndFeel::Spacing::groupTitleBand);
                    if (group.getBounds().contains (other.getBounds()) && band.intersects (other.getBounds()))
                        fail (scenario + ": control runs into the title band of group \"" + group.getName() + "\": " + describe (other));
                }
            }
        });

        // The column must be tall enough to hold everything in it.
        if (column != nullptr)
        {
            int lowest = 0;
            for (auto* child : column->getChildren())
                if (isOnScreen (*child)) lowest = juce::jmax (lowest, child->getBottom());
            if (lowest > column->getHeight())
                fail (scenario + ": content (" + juce::String (lowest) + " px) is taller than its scrolling column");
        }

        std::cout << (failures == failuresBefore ? "  [ ok ] " : "  [FAIL] ") << scenario << std::endl;
    }

    void collectDescendants (juce::Component& root, std::vector<CollapsiblePanel*>& out)
    {
        for (auto* child : root.getChildren())
        {
            if (auto* panel = dynamic_cast<CollapsiblePanel*> (child))
                out.push_back (panel);
            collectDescendants (*child, out);
        }
    }

    juce::File makeWav (int channels)
    {
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("zqsfx_layout_test").getChildFile ("fixture_" + juce::String (channels) + "ch.wav");
        file.getParentDirectory().createDirectory();
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), 48000.0,
                                                                            (unsigned int) channels, 16, {}, 0));
        juce::AudioBuffer<float> buffer (channels, 480);
        buffer.clear();
        writer->writeFromAudioSampleBuffer (buffer, 0, 480);
        return file;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    std::cout << "=== ZQ SFX LayoutTest ===" << std::endl;

    MainComponent::ScopedHouseLookAndFeel look;
    ConfigManager config; // defaults only; never touches disk

    // 5. the shipped window limits fit a 13-inch laptop (about 1280x775 usable, less the title bar)
    if (MainComponent::kMinWidth > 1200 || MainComponent::kMinHeight > 760
        || MainComponent::kDefaultWidth > 1200 || MainComponent::kDefaultHeight > 760)
        fail ("window minimum/default do not fit a 13-inch laptop");
    else
        std::cout << "  [ ok ] window minimum " << MainComponent::kMinWidth << "x" << MainComponent::kMinHeight
                  << " and default " << MainComponent::kDefaultWidth << "x" << MainComponent::kDefaultHeight
                  << " fit a 13-inch laptop" << std::endl;

    struct Size { int w, h; };
    const Size sizes[] = { { 1000, 850 }, { 960, 640 }, { 1000, 700 }, { 1180, 740 }, { 1440, 900 } };

    const auto file32 = makeWav (32);
    const auto file8 = makeWav (8);

    for (auto size : sizes)
    {
        const juce::String sz = juce::String (size.w) + "x" + juce::String (size.h);

        {
            MainComponent main (&config);
            main.setSize (size.w, size.h);
            checkLayout (main, sz + " single-file tab, no file");

            main.getSingleFileSplitter()->filesDropped (juce::StringArray (file8.getFullPathName()), 0, 0);
            checkLayout (main, sz + " single-file tab, 8-channel file");

            main.getSingleFileSplitter()->filesDropped (juce::StringArray (file32.getFullPathName()), 0, 0);
            checkLayout (main, sz + " single-file tab, 32-channel file");

            // Collapsing either panel (or both) must leave a clean layout, and so must expanding again.
            std::vector<CollapsiblePanel*> panels;
            collectDescendants (main, panels);
            if (panels.size() != 2)
                fail (sz + ": expected 2 collapsible panels, found " + juce::String ((int) panels.size()));
            for (auto* panel : panels)
            {
                panel->setCollapsed (true, false);
                checkLayout (main, sz + " one panel collapsed (" + panel->getName() + ")");
            }
            checkLayout (main, sz + " both panels collapsed");
            for (auto* panel : panels)
                panel->setCollapsed (false, false);
            checkLayout (main, sz + " panels expanded again");

            main.setCurrentTab (1);
            checkLayout (main, sz + " batch tab");

            main.getProgressPanel()->updateBatchProgress (0.4, "Some_Long_File_Name_48kHz.wav", 3, 10);
            checkLayout (main, sz + " batch tab, batch progress showing");
        }
    }

    juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zqsfx_layout_test").deleteRecursively();

    std::cout << "\n" << checks << " component checks\n=== " << (failures == 0 ? "PASS" : "FAIL")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
