#pragma once

#include <JuceHeader.h>
#include <functional>

//==============================================================================
/**
 * UserNotice - the one place the splitter tabs show a message to the user (validation problems,
 * "completed", "cancelled", "failed"). By default it opens an asynchronous alert window.
 * Tests replace handler() to capture the messages instead of opening windows, so the whole
 * start / cancel / complete flow can run headless.
 */
namespace UserNotice
{
    using Handler = std::function<void(const juce::MessageBoxOptions&)>;

    inline Handler& handler()
    {
        static Handler current = [](const juce::MessageBoxOptions& options)
        {
            juce::AlertWindow::showAsync(options, nullptr);
        };
        return current;
    }

    inline void show(const juce::MessageBoxOptions& options)
    {
        if (auto& h = handler())
            h(options);
    }
}
