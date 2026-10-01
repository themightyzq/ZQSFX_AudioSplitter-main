#pragma once

#include <JuceHeader.h>
#include "../Utils/ConfigManager.h"
#include "../Utils/AudioAnalyzer.h"
#include "../Utils/AudioFileProcessor.h"

// Forward declarations
class MainComponent;

//==============================================================================
/**
 * Single File Splitter Component - mirrors Python single file tab
 * 
 * Python equivalent: Lines 1312-1453 (Single File Tab setup)
 * Responsible for:
 * - Single file selection and validation
 * - Channel selection UI
 * - Output directory selection
 * - Single file processing workflow
 */
class SingleFileSplitter : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    //==============================================================================
    explicit SingleFileSplitter(ConfigManager* configManager);
    ~SingleFileSplitter() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // FileDragAndDropTarget interface - mirrors Python drag-and-drop (lines 1344-1350)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    //==============================================================================
    // Interface for MainComponent - mirrors Python variable access
    
    /**
     * Get selected file path - mirrors Python single_file_var.get()
     */
    juce::String getSelectedFile() const;
    
    /**
     * Get output directory - mirrors Python output_dir_var.get()
     */
    juce::String getOutputDirectory() const;
    
    /**
     * Start processing - mirrors Python split_single_file() (lines 673-828). Validation problems
     * are shown to the user and nothing starts; check isBusy() afterwards.
     */
    void startProcessing();

    /**
     * Height this tab needs at `width` to show every control and every channel button without
     * clipping. The window scrolls when the sum of its parts exceeds the screen; it never
     * squeezes this content. onPreferredHeightChanged fires when it changes (a file with a
     * different channel count was loaded).
     */
    int getPreferredHeight(int width) const;

    std::function<void()> onPreferredHeightChanged;

    /** True from the moment a split starts until its completion callback has run. */
    bool isBusy() const;

    /** Ask the running split to stop (non-blocking). No-op when idle. */
    void cancelProcessing();
    
    //==============================================================================
    // Callback for UI updates - mirrors Python update_button_states() calls
    std::function<void()> onFileAnalyzed;

    /** Called whenever isBusy() changes, so the window can enable or disable Split. */
    std::function<void()> onBusyChanged;

private:
    //==============================================================================
    // UI Layout Constants (Phase 2 UI/UX improvement)
    static constexpr int kHelperButtonWidth = 72;
    static constexpr int kMargin = 12;
    static constexpr int kRowHeight = 32;
    static constexpr int kGap = 6;
    static constexpr int kGroupGap = 10;
    static constexpr int kGroupPadding = 8;
    static constexpr int kHeaderRowHeight = 30;
    static constexpr int kChannelCellMinWidth = 118;
    static constexpr int kChannelCellHeight = 28;
    static constexpr int kChannelCellGap = 4;
    static constexpr int kFileButtonWidth = 120;
    static constexpr int kLabelWidth = 140;

    /** Rows of channel buttons needed at the given width of the grid area. */
    int channelGridRows(int gridWidth) const;

    //==============================================================================
    // UI Components - mirrors Python single file UI structure
    
    // File selection - mirrors lines 1327-1367
    juce::Label fileLabel;
    juce::TextEditor filePathEditor;
    juce::TextButton browseFileButton;
    juce::TextButton openFileButton;
    
    // Output directory - mirrors lines 1369-1409
    juce::Label outputDirLabel;
    juce::TextEditor outputDirEditor;
    juce::TextButton browseOutputButton;
    juce::TextButton openOutputButton;
    
    // Channel selection - mirrors lines 1411-1453
    juce::GroupComponent channelGroup;
    juce::Label channelSelectionLabel;
    juce::OwnedArray<juce::ToggleButton> channelButtons;

    // Channel selection helpers (Phase 2 UI/UX improvement)
    juce::TextButton selectAllButton;
    juce::TextButton selectNoneButton;
    juce::TextButton invertSelectionButton;
    juce::Label channelCountLabel;
    
    //==============================================================================
    // State management
    ConfigManager* configManager;
    AudioAnalyzer audioAnalyzer;
    std::unique_ptr<AudioFileProcessor> audioProcessor;
    juce::String currentFilePath;
    juce::String currentOutputDir;
    int detectedChannels {0};
    AudioAnalyzer::AudioFileInfo currentFileInfo;
    
    //==============================================================================
    // Helper methods - mirror Python helper functions
    void updateChannelButtons();      // mirrors update_channel_checkboxes() (lines 1016-1048)
    void browseForFile();            // mirrors browse_single_file() (lines 657-671)
    void browseForOutputDir();       // mirrors browse_output_dir() (lines 459-472)
    void openFileLocation();         // mirrors open_file_directory() (lines 598-610)
    void openOutputLocation();       // mirrors open_output_directory() (lines 429-441)

    // Channel selection helper methods (Phase 2 UI/UX improvement)
    void selectAllChannels();        // Select all available channels
    void selectNoChannels();         // Deselect all channels
    void invertChannelSelection();   // Invert current selection
    void updateChannelCountLabel();  // Update "X of Y channels selected" label
    
    //==============================================================================
    // Audio processing methods
    std::vector<int> getSelectedChannels() const;  // Get list of selected channel indices
    void onProcessingProgress(double progress, const juce::String& message);
    void onProcessingComplete(const AudioFileProcessor::ProcessingResult& result);
    
    // Set on the message thread when a split starts, cleared by its completion callback. Not
    // derived from the worker thread: the thread can outlive the completion callback briefly.
    bool running {false};

    // File choosers - need to be member variables to stay in scope
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> outputDirChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleFileSplitter)
};