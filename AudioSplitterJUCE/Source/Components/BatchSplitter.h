#pragma once

#include <JuceHeader.h>
#include "../Utils/ConfigManager.h"
#include "../Utils/AudioAnalyzer.h"
#include "../Utils/AudioFileProcessor.h"
#include "../Utils/BatchJobManager.h"
#include "../Utils/BatchRun.h"
#include "../Utils/FolderScanner.h"

//==============================================================================
/**
 * Batch Splitter Component - Parallel batch processing with thread pool
 *
 * Architecture:
 * - A BatchRun owns the worker pool (4-8 threads), the per-file jobs and their manager, and
 *   knows how to take them down safely; this component only starts it, cancels it and shows
 *   its progress and result.
 * - Each file gets an independent BatchProcessingJob with its own AudioFileProcessor
 * - Progress aggregated from all jobs; errors collected from failed jobs
 *
 * Thread Safety:
 * - Callbacks arrive on the message thread and are bound through a SafePointer, so one that is
 *   still queued when this component dies is dropped
 * - The destructor drains the workers (BatchRun) before anything they use is freed
 */
class BatchSplitter : public juce::Component,
                     public juce::FileDragAndDropTarget
{
public:
    //==============================================================================
    explicit BatchSplitter(ConfigManager* configManager);
    ~BatchSplitter() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // FileDragAndDropTarget interface - mirrors Python drag-and-drop for batch
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    //==============================================================================
    // Interface for MainComponent - mirrors Python variable access
    
    /**
     * Get input directory - mirrors Python batch_input_dir_var.get()
     */
    juce::String getInputDirectory() const;
    
    /**
     * Get output directory - mirrors Python batch_output_dir_var.get()
     */
    juce::String getOutputDirectory() const;
    
    /**
     * Start batch processing - mirrors Python split_batch_files() (lines 830-887)
     */
    void startProcessing();

    /** Height this tab needs to show all of its controls (fixed; see SingleFileSplitter). */
    int getPreferredHeight() const;

    /** True while the input folder is being read in the background (Split must wait for it). */
    bool isScanning() const { return scanner != nullptr; }

    /** True from the moment a batch starts until its completion has been handled. */
    bool isBusy() const { return batchRun != nullptr; }

    /** Ask the running batch to stop (non-blocking). No-op when idle. */
    void cancelProcessing();

    /**
     * Update file count display - mirrors Python update_file_count() (lines 474-480)
     */
    void updateFileCount();
    
    //==============================================================================
    // Callback for UI updates - mirrors Python update_button_states() calls
    std::function<void()> onDirectoryAnalyzed;

    /** Called whenever isBusy() changes, so the window can enable or disable Split. */
    std::function<void()> onBusyChanged;

private:
    //==============================================================================
    // Layout constants shared by resized() and getPreferredHeight()
    static constexpr int kMargin = 12;
    static constexpr int kRowHeight = 32;
    static constexpr int kLabelWidth = 140;
    static constexpr int kButtonWidth = 120;

    //==============================================================================
    // UI Components - mirrors Python batch UI structure
    
    // Input directory - mirrors lines 1470-1510
    juce::Label inputDirLabel;
    juce::TextEditor inputDirEditor;
    juce::TextButton browseInputButton;
    juce::TextButton openInputButton;
    
    // Output directory - mirrors lines 1512-1552
    juce::Label outputDirLabel;
    juce::TextEditor outputDirEditor;
    juce::TextButton browseOutputButton;
    juce::TextButton openOutputButton;
    
    // File count display - mirrors lines 1554-1556
    juce::Label fileCountLabel;
    
    //==============================================================================
    // State management
    ConfigManager* configManager;
    juce::String currentInputDir;
    juce::String currentOutputDir;
    int fileCount {0};

    // The readable WAV files in the input folder and their channel counts, filled by the
    // background scan. Starting a batch reads nothing from disk on the message thread.
    std::vector<FolderScanner::Entry> scannedFiles;
    std::unique_ptr<FolderScanner> scanner;   // non-null while a scan is running

    // The running batch, or null when idle. Destroyed (draining its workers) in ~BatchSplitter.
    std::unique_ptr<BatchRun> batchRun;

    //==============================================================================
    // Helper methods
    void browseForInputDir();
    void browseForOutputDir();
    void openInputLocation();
    void openOutputLocation();
    void scanInputFolder();
    void onScanProgress(int scanned, int total);
    void onScanFinished(std::vector<FolderScanner::Entry> entries, int skipped);

    // Parallel batch processing callbacks
    void onBatchProgress(double overallProgress, const juce::String& currentFile,
                        int completedCount, int totalCount);
    void onBatchComplete(const BatchJobManager::BatchResult& result);
    
    // File choosers - need to be member variables to stay in scope
    std::unique_ptr<juce::FileChooser> inputDirChooser;
    std::unique_ptr<juce::FileChooser> outputDirChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchSplitter)
};