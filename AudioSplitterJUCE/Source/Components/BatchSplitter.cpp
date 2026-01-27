#include "BatchSplitter.h"
#include "../MainComponent.h"
#include "../Utils/AudioFileProcessor.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
BatchSplitter::BatchSplitter(ConfigManager* config)
    : configManager(config),
      inputDirLabel("inputDirLabel", "Input Directory:"),
      browseInputButton("Browse..."),
      openInputButton("Open Location"),
      outputDirLabel("outputDirLabel", "Output Directory:"),
      browseOutputButton("Browse..."),
      openOutputButton("Open Location"),
      fileCountLabel("fileCountLabel", "No files found")
{
    // Setup input directory UI - mirrors Python lines 1470-1510
    inputDirLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(inputDirLabel);

    inputDirEditor.setReadOnly(true);
    inputDirEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    inputDirEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(inputDirEditor);
    
    browseInputButton.onClick = [this]() { browseForInputDir(); };
    addAndMakeVisible(browseInputButton);
    
    openInputButton.onClick = [this]() { openInputLocation(); };
    addAndMakeVisible(openInputButton);
    
    // Setup output directory UI - mirrors Python lines 1512-1552
    outputDirLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(outputDirLabel);

    outputDirEditor.setReadOnly(true);
    outputDirEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    outputDirEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(outputDirEditor);
    
    browseOutputButton.onClick = [this]() { browseForOutputDir(); };
    addAndMakeVisible(browseOutputButton);
    
    openOutputButton.onClick = [this]() { openOutputLocation(); };
    addAndMakeVisible(openOutputButton);
    
    // Setup file count display - mirrors Python lines 1554-1556
    fileCountLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    fileCountLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fileCountLabel);
    
    // Initialize with config defaults
    if (configManager)
    {
        currentInputDir = configManager->getLastInputDir();
        currentOutputDir = configManager->getLastOutputDir();
        inputDirEditor.setText(currentInputDir);
        outputDirEditor.setText(currentOutputDir);
        
        // Update file count if input directory is set
        if (!currentInputDir.isEmpty())
        {
            updateFileCount();
        }
    }
    
    juce::Logger::writeToLog("BatchSplitter initialized");
}

//==============================================================================
void BatchSplitter::paint(juce::Graphics& g)
{
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
void BatchSplitter::resized()
{
    auto bounds = getLocalBounds();
    const int margin = 10;
    const int buttonWidth = 100;
    const int rowHeight = 30;
    const int spacing = 5;
    
    bounds.reduce(margin, margin);
    
    // Input directory row - mirrors Python grid layout
    auto inputRow = bounds.removeFromTop(rowHeight);
    inputDirLabel.setBounds(inputRow.removeFromLeft(120));
    openInputButton.setBounds(inputRow.removeFromRight(buttonWidth));
    inputRow.removeFromRight(spacing);
    browseInputButton.setBounds(inputRow.removeFromRight(buttonWidth));
    inputRow.removeFromRight(spacing);
    inputDirEditor.setBounds(inputRow);
    
    bounds.removeFromTop(spacing);
    
    // Output directory row - mirrors Python grid layout
    auto outputRow = bounds.removeFromTop(rowHeight);
    outputDirLabel.setBounds(outputRow.removeFromLeft(120));
    openOutputButton.setBounds(outputRow.removeFromRight(buttonWidth));
    outputRow.removeFromRight(spacing);
    browseOutputButton.setBounds(outputRow.removeFromRight(buttonWidth));
    outputRow.removeFromRight(spacing);
    outputDirEditor.setBounds(outputRow);
    
    bounds.removeFromTop(spacing * 2);
    
    // File count label - mirrors Python file count display
    fileCountLabel.setBounds(bounds.removeFromTop(rowHeight));
}

//==============================================================================
// FileDragAndDropTarget interface - mirrors Python drag-and-drop for batch
bool BatchSplitter::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Accept directories or multiple WAV files
    if (files.size() == 1)
    {
        return juce::File(files[0]).isDirectory();
    }
    
    // Check if all files are WAV files
    for (const auto& file : files)
    {
        if (!file.toLowerCase().endsWithIgnoreCase(".wav"))
            return false;
    }
    
    return files.size() > 1;
}

void BatchSplitter::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    repaint(); // Visual feedback
}

void BatchSplitter::fileDragExit(const juce::StringArray& /*files*/)
{
    repaint(); // Remove visual feedback
}

void BatchSplitter::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.size() == 1 && juce::File(files[0]).isDirectory())
    {
        // Directory dropped
        currentInputDir = files[0];
        inputDirEditor.setText(currentInputDir);
        
        if (configManager)
        {
            configManager->setLastInputDir(currentInputDir);
        }
        
        updateFileCount();
        juce::Logger::writeToLog("Directory dropped: " + currentInputDir);
    }
}

//==============================================================================
juce::String BatchSplitter::getInputDirectory() const
{
    return currentInputDir;
}

juce::String BatchSplitter::getOutputDirectory() const
{
    return currentOutputDir;
}

void BatchSplitter::startProcessing()
{
    // Validate inputs
    if (currentInputDir.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "Invalid Input",
                                                             "Please select an input directory.");
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    if (currentOutputDir.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "Invalid Output",
                                                             "Please select an output directory.");
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    if (validAudioFiles.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "No Files",
                                                             "No WAV files found in the input directory.");
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }

    const int totalFiles = validAudioFiles.size();
    juce::Logger::writeToLog("Starting parallel batch processing of " + juce::String(totalFiles) + " files");

    // Create BatchJobManager with callbacks
    batchJobManager = std::make_unique<BatchJobManager>(
        totalFiles,
        [this](double progress, const juce::String& currentFile, int completed, int total)
        {
            onBatchProgress(progress, currentFile, completed, total);
        },
        [this](const BatchJobManager::BatchResult& result)
        {
            onBatchComplete(result);
        }
    );

    // Determine optimal thread count (4-8 workers based on CPU cores)
    const int numCores = juce::SystemStats::getNumCpus();
    const int numWorkers = juce::jlimit(4, 8, numCores);

    threadPool = std::make_unique<juce::ThreadPool>(numWorkers);
    juce::Logger::writeToLog("Created thread pool with " + juce::String(numWorkers) + " workers");

    // Clear previous jobs
    batchJobs.clear();

    // Create and submit all jobs
    for (int i = 0; i < totalFiles; ++i)
    {
        const auto& file = validAudioFiles[i];

        // Build processing options
        AudioFileProcessor::ProcessingOptions options;
        options.inputFilePath = file.getFullPathName();
        options.outputDirectory = currentOutputDir;

        // Get all channels for this file
        auto fileInfo = audioAnalyzer.analyzeFile(options.inputFilePath);
        if (fileInfo.isValid)
        {
            options.selectedChannels.clear();
            for (int ch = 0; ch < fileInfo.numChannels; ++ch)
            {
                options.selectedChannels.push_back(ch);
            }
        }

        // Get settings from OptionsPanel
        if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
        {
            if (auto* optionsPanel = mainComponent->getOptionsPanel())
            {
                options.customChannelNames = optionsPanel->getCustomNames();

                if (optionsPanel->getOverrideSampleRate())
                {
                    auto sampleRateStr = optionsPanel->getSampleRate();
                    if (sampleRateStr.contains("Hz"))
                        options.sampleRate = sampleRateStr.getDoubleValue();
                    else
                        options.sampleRate = 0.0;
                }

                if (optionsPanel->getOverrideBitDepth())
                {
                    auto bitDepthStr = optionsPanel->getBitDepth();
                    if (bitDepthStr.contains("bit"))
                        options.bitDepth = bitDepthStr.getIntValue();
                    else
                        options.bitDepth = 0;
                }

                options.preserveMetadata = optionsPanel->getPreserveIXML();
                options.stereoToMono = optionsPanel->getStereoToMono();
            }
        }

        // Create job and add to thread pool
        auto* job = new BatchProcessingJob(options, i, batchJobManager.get());
        batchJobs.add(job);
        threadPool->addJob(job, false);  // false = don't delete job when done (we manage it)

        juce::Logger::writeToLog("Submitted job " + juce::String(i) + ": " + file.getFileName());
    }

    juce::Logger::writeToLog("All " + juce::String(totalFiles) + " jobs submitted to thread pool");
}

void BatchSplitter::updateFileCount()
{
    // mirrors update_file_count() (lines 474-480)
    countWavFiles();
    
    juce::String countText;
    if (fileCount == 0)
    {
        countText = "No WAV files found";
    }
    else if (fileCount == 1)
    {
        countText = "1 WAV file found";
    }
    else
    {
        countText = juce::String(fileCount) + " WAV files found";
    }
    
    fileCountLabel.setText(countText, juce::dontSendNotification);
    juce::Logger::writeToLog("File count updated: " + countText);
    
    // Notify main component that directory analysis is complete
    if (onDirectoryAnalyzed)
        onDirectoryAnalyzed();
}

//==============================================================================
// Helper methods - mirror Python helper functions
void BatchSplitter::browseForInputDir()
{
    // mirrors browse_batch_input_dir() (lines 443-457)
    inputDirChooser = std::make_unique<juce::FileChooser>("Select Input Directory", 
                                                         juce::File(configManager ? configManager->getLastInputDir() : ""));
    
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectDirectories;
    
    inputDirChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto dir = fc.getResult();
        if (dir != juce::File{})
        {
            currentInputDir = dir.getFullPathName();
            inputDirEditor.setText(currentInputDir);
            
            if (configManager)
            {
                configManager->setLastInputDir(currentInputDir);
            }
            
            updateFileCount();
            juce::Logger::writeToLog("Input directory selected: " + currentInputDir);
        }
    });
}

void BatchSplitter::browseForOutputDir()
{
    // mirrors browse_batch_output_dir() (lines 459-472)
    outputDirChooser = std::make_unique<juce::FileChooser>("Select Output Directory", 
                                                          juce::File(configManager ? configManager->getLastOutputDir() : ""));
    
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectDirectories;
    
    outputDirChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto dir = fc.getResult();
        if (dir != juce::File{})
        {
            currentOutputDir = dir.getFullPathName();
            outputDirEditor.setText(currentOutputDir);
            
            if (configManager)
            {
                configManager->setLastOutputDir(currentOutputDir);
            }
            
            juce::Logger::writeToLog("Output directory selected: " + currentOutputDir);
        }
    });
}

void BatchSplitter::openInputLocation()
{
    // mirrors open_batch_input_directory() (lines 482-494)
    if (!currentInputDir.isEmpty())
    {
        juce::File(currentInputDir).revealToUser();
    }
}

void BatchSplitter::openOutputLocation()
{
    // mirrors open_batch_output_directory() (lines 496-508)
    if (!currentOutputDir.isEmpty())
    {
        juce::File(currentOutputDir).revealToUser();
    }
}

void BatchSplitter::countWavFiles()
{
    // mirrors WAV file counting logic (lines 476-480)
    fileCount = 0;
    validAudioFiles.clear();
    
    if (currentInputDir.isEmpty())
        return;
    
    juce::File inputDir(currentInputDir);
    if (!inputDir.isDirectory())
        return;
    
    // Find WAV files and validate them - mirrors Python os.listdir() + .endswith(".wav") (line 477)
    auto files = inputDir.findChildFiles(juce::File::findFiles, false, "*.wav");
    
    for (const auto& file : files)
    {
        // Validate each file using AudioAnalyzer - more thorough than Python version
        if (audioAnalyzer.isValidAudioFile(file.getFullPathName()))
        {
            validAudioFiles.add(file);
            fileCount++;
            juce::Logger::writeToLog("Valid WAV file found: " + file.getFileName());
        }
        else
        {
            juce::Logger::writeToLog("Invalid or corrupted WAV file skipped: " + file.getFileName());
        }
    }
    
    juce::Logger::writeToLog("Found " + juce::String(fileCount) + " valid WAV files in directory");
}

//==============================================================================
// Parallel batch processing callbacks

void BatchSplitter::onBatchProgress(double overallProgress, const juce::String& currentFile,
                                    int completedCount, int totalCount)
{
    // Update progress display on UI thread (this is already called via MessageManager::callAsync)
    if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
    {
        if (auto* progressPanel = mainComponent->getProgressPanel())
        {
            // Use enhanced batch progress reporting (Phase 2.2)
            progressPanel->updateBatchProgress(overallProgress, currentFile, completedCount, totalCount);
        }
    }

    juce::Logger::writeToLog("Batch progress: " + juce::String(overallProgress * 100.0, 1) + "% (" +
                            juce::String(completedCount) + "/" + juce::String(totalCount) + ")");
}

void BatchSplitter::onBatchComplete(const BatchJobManager::BatchResult& result)
{
    // Build completion message
    juce::String message;

    if (result.wasCancelled)
    {
        message = "Batch processing cancelled by user.\n\n";
    }
    else
    {
        message = "Batch processing completed!\n\n";
    }

    message += "Total files: " + juce::String(result.totalFiles) + "\n";
    message += "Successful: " + juce::String(result.successfulFiles) + "\n";
    message += "Failed: " + juce::String(result.failedFiles) + "\n";
    message += "Total time: " + juce::String(result.totalProcessingTimeSeconds, 1) + " seconds\n";

    if (result.failedFiles > 0)
    {
        message += "\nFailed files:\n";
        for (const auto& error : result.errorMessages)
        {
            message += "  " + error + "\n";
        }
    }

    auto messageType = result.wasCancelled ? juce::MessageBoxIconType::WarningIcon :
                       (result.failedFiles == 0) ? juce::MessageBoxIconType::InfoIcon :
                       juce::MessageBoxIconType::WarningIcon;

    auto options = juce::MessageBoxOptions::makeOptionsOk(messageType,
                                                         "Batch Processing Complete",
                                                         message);
    juce::AlertWindow::showAsync(options, nullptr);

    // Reset progress panel
    if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
    {
        if (auto* progressPanel = mainComponent->getProgressPanel())
        {
            progressPanel->updateProgressSafely(0.0, "Ready");
        }
    }

    juce::Logger::writeToLog("Batch processing complete: " + juce::String(result.successfulFiles) +
                            " succeeded, " + juce::String(result.failedFiles) + " failed");

    // Clean up
    batchJobs.clear();
    threadPool.reset();
    batchJobManager.reset();
}