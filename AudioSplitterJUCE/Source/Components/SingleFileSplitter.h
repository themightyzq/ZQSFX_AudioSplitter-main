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
     * Start processing - mirrors Python split_single_file() (lines 673-828)
     */
    void startProcessing();
    
    //==============================================================================
    // Callback for UI updates - mirrors Python update_button_states() calls
    std::function<void()> onFileAnalyzed;

private:
    //==============================================================================
    // UI Layout Constants (Phase 2 UI/UX improvement)
    static constexpr int kHelperButtonWidth = 80;

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
    
    // File choosers - need to be member variables to stay in scope
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> outputDirChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleFileSplitter)
};