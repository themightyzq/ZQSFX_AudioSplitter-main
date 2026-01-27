#pragma once

#include <JuceHeader.h>
#include "../Utils/ConfigManager.h"
#include "../Utils/AudioAnalyzer.h"
#include "../Utils/AudioFileProcessor.h"
#include "../Utils/BatchJobManager.h"
#include "../Utils/BatchProcessingJob.h"

//==============================================================================
/**
 * Batch Splitter Component - Parallel batch processing with thread pool
 *
 * Architecture:
 * - Uses juce::ThreadPool for parallel processing (4-8 workers)
 * - BatchJobManager coordinates all jobs thread-safely
 * - Each file gets independent BatchProcessingJob with own AudioFileProcessor
 * - Progress aggregated from all jobs
 * - Errors collected from failed jobs
 *
 * Thread Safety:
 * - UI updates via MessageManager::callAsync
 * - Progress tracking via atomic operations
 * - Error collection via mutex protection
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
    
    /**
     * Update file count display - mirrors Python update_file_count() (lines 474-480)
     */
    void updateFileCount();
    
    //==============================================================================
    // Callback for UI updates - mirrors Python update_button_states() calls
    std::function<void()> onDirectoryAnalyzed;

private:
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
    AudioAnalyzer audioAnalyzer;
    juce::String currentInputDir;
    juce::String currentOutputDir;
    int fileCount {0};
    juce::Array<juce::File> validAudioFiles;

    // Parallel processing components
    std::unique_ptr<juce::ThreadPool> threadPool;
    std::unique_ptr<BatchJobManager> batchJobManager;
    juce::OwnedArray<BatchProcessingJob> batchJobs;  // Keep jobs alive during processing

    // Prevent dangling this in async callbacks
    std::shared_ptr<std::atomic<bool>> aliveFlag = std::make_shared<std::atomic<bool>>(true);
    std::atomic<bool> isProcessing {false};

    //==============================================================================
    // Helper methods
    void browseForInputDir();
    void browseForOutputDir();
    void openInputLocation();
    void openOutputLocation();
    void countWavFiles();

    // Parallel batch processing callbacks
    void onBatchProgress(double overallProgress, const juce::String& currentFile,
                        int completedCount, int totalCount);
    void onBatchComplete(const BatchJobManager::BatchResult& result);
    
    // File choosers - need to be member variables to stay in scope
    std::unique_ptr<juce::FileChooser> inputDirChooser;
    std::unique_ptr<juce::FileChooser> outputDirChooser;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BatchSplitter)
};