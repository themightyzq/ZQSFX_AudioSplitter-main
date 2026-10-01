#include "BatchSplitter.h"
#include "../MainComponent.h"
#include "../Utils/AudioFileProcessor.h"
#include "../UI/ModernLookAndFeel.h"
#include "../Utils/UserNotice.h"

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
    inputDirEditor.setColour(juce::TextEditor::outlineColourId, ModernLookAndFeel::Colors::border);
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
    outputDirEditor.setColour(juce::TextEditor::outlineColourId, ModernLookAndFeel::Colors::border);
    addAndMakeVisible(outputDirEditor);
    
    browseOutputButton.onClick = [this]() { browseForOutputDir(); };
    addAndMakeVisible(browseOutputButton);
    
    openOutputButton.onClick = [this]() { openOutputLocation(); };
    addAndMakeVisible(openOutputButton);
    
    // Setup file count display - mirrors Python lines 1554-1556
    fileCountLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    fileCountLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fileCountLabel);

    // Add tooltips for better UX
    browseInputButton.setTooltip("Browse for input directory containing WAV files");
    openInputButton.setTooltip("Open input directory in file browser");
    browseOutputButton.setTooltip("Browse for output directory");
    openOutputButton.setTooltip("Open output directory in file browser");

    // Accessibility floor: visible label + description on every button (style guide section 8).
    browseInputButton.setTitle("Browse for Input Directory");
    browseInputButton.setDescription("Browse for input directory containing WAV files");
    openInputButton.setTitle("Open Input Location");
    openInputButton.setDescription("Open input directory in file browser");
    browseOutputButton.setTitle("Browse for Output Directory");
    browseOutputButton.setDescription("Browse for output directory");
    openOutputButton.setTitle("Open Output Location");
    openOutputButton.setDescription("Open output directory in file browser");
    
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
BatchSplitter::~BatchSplitter()
{
    // Quitting mid-batch: ~BatchRun cancels, waits for every worker to stop and only then frees
    // the jobs and manager they were using. Nothing queued can reach this component afterwards.
    batchRun.reset();
    scanner.reset();
}

//==============================================================================
void BatchSplitter::paint(juce::Graphics& g)
{
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
void BatchSplitter::resized()
{
    const int spacing = ModernLookAndFeel::Spacing::sm;
    auto bounds = getLocalBounds().reduced(kMargin);

    // Input directory row - mirrors Python grid layout
    auto inputRow = bounds.removeFromTop(kRowHeight);
    inputDirLabel.setBounds(inputRow.removeFromLeft(kLabelWidth));
    openInputButton.setBounds(inputRow.removeFromRight(kButtonWidth));
    inputRow.removeFromRight(spacing);
    browseInputButton.setBounds(inputRow.removeFromRight(kButtonWidth));
    inputRow.removeFromRight(spacing);
    inputDirEditor.setBounds(inputRow);

    bounds.removeFromTop(spacing);

    // Output directory row - mirrors Python grid layout
    auto outputRow = bounds.removeFromTop(kRowHeight);
    outputDirLabel.setBounds(outputRow.removeFromLeft(kLabelWidth));
    openOutputButton.setBounds(outputRow.removeFromRight(kButtonWidth));
    outputRow.removeFromRight(spacing);
    browseOutputButton.setBounds(outputRow.removeFromRight(kButtonWidth));
    outputRow.removeFromRight(spacing);
    outputDirEditor.setBounds(outputRow);

    bounds.removeFromTop(spacing * 2);

    // File count label - mirrors Python file count display
    fileCountLabel.setBounds(bounds.removeFromTop(kRowHeight));
}

int BatchSplitter::getPreferredHeight() const
{
    // Mirrors resized(): margin, two rows with a gap, a double gap, the count row, margin.
    return kMargin + kRowHeight + ModernLookAndFeel::Spacing::sm + kRowHeight
         + ModernLookAndFeel::Spacing::sm * 2 + kRowHeight + kMargin;
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
        UserNotice::show(options);
        return;
    }

    if (currentOutputDir.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "Invalid Output",
                                                             "Please select an output directory.");
        UserNotice::show(options);
        return;
    }

    if (scanner != nullptr)
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::InfoIcon,
                                                             "Still Scanning",
                                                             "The input folder is still being read. Try again in a moment.");
        UserNotice::show(options);
        return;
    }

    if (scannedFiles.empty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "No Files",
                                                             "No WAV files found in the input directory.");
        UserNotice::show(options);
        return;
    }

    if (batchRun != nullptr)
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::InfoIcon,
                                                             "Batch In Progress",
                                                             "A batch is already running. Wait for it to finish or cancel it.");
        UserNotice::show(options);
        return;
    }

    const int totalFiles = (int) scannedFiles.size();
    juce::Logger::writeToLog("Starting parallel batch processing of " + juce::String(totalFiles) + " files");

    auto* mainComponent = findParentComponentOfClass<MainComponent>();
    const OptionsPanel* optionsPanel = mainComponent != nullptr ? mainComponent->getOptionsPanel() : nullptr;

    std::vector<AudioFileProcessor::ProcessingOptions> jobOptions;
    jobOptions.reserve((size_t) totalFiles);

    for (const auto& scanned : scannedFiles)
    {
        AudioFileProcessor::ProcessingOptions options;
        options.inputFilePath = scanned.file.getFullPathName();
        options.outputDirectory = currentOutputDir;

        // Every channel of the file, as counted by the background scan (no disk access here).
        for (int ch = 0; ch < scanned.numChannels; ++ch)
            options.selectedChannels.push_back(ch);

        // Same panel reading as the single-file tab. UCS naming is deliberately not applied
        // here: one category/description across many files would give every file the same
        // output names. Batch outputs are named <source>_<channel>.
        if (optionsPanel != nullptr)
            optionsPanel->applyTo(options);

        jobOptions.push_back(options);
    }

    // 4-8 workers based on CPU cores
    const int numWorkers = juce::jlimit(4, 8, juce::SystemStats::getNumCpus());
    juce::Logger::writeToLog("Created thread pool with " + juce::String(numWorkers) + " workers");

    // Both callbacks run on the message thread. The SafePointer drops one that is queued when
    // this component is destroyed; BatchRun drops them once the run itself is gone.
    juce::Component::SafePointer<BatchSplitter> weakThis(this);
    batchRun = std::make_unique<BatchRun>(
        jobOptions, numWorkers,
        [weakThis](double progress, const juce::String& currentFile, int completed, int total)
        {
            if (auto* self = weakThis.getComponent())
                self->onBatchProgress(progress, currentFile, completed, total);
        },
        [weakThis](const BatchJobManager::BatchResult& result)
        {
            if (auto* self = weakThis.getComponent())
                self->onBatchComplete(result);
        });

    // Show batch mode (and its Cancel button) immediately rather than at the first report.
    if (mainComponent != nullptr)
    {
        if (auto* progressPanel = mainComponent->getProgressPanel())
            progressPanel->updateBatchProgress(0.0, {}, 0, totalFiles);
    }

    juce::Logger::writeToLog("All " + juce::String(totalFiles) + " jobs submitted to thread pool");

    if (onBusyChanged)
        onBusyChanged();
}

void BatchSplitter::cancelProcessing()
{
    if (batchRun != nullptr)
        batchRun->cancel();
}

void BatchSplitter::updateFileCount()
{
    // mirrors update_file_count() (lines 474-480), but the folder is read on a background
    // thread: a folder of thousands of files, or one on a slow drive, must not freeze the window.
    scanInputFolder();
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

void BatchSplitter::scanInputFolder()
{
    // A newer choice supersedes a scan still running: destroying it joins its thread and drops
    // any of its callbacks that are already queued.
    scanner.reset();
    scannedFiles.clear();
    fileCount = 0;

    juce::File inputDir(currentInputDir);
    if (currentInputDir.isEmpty() || !inputDir.isDirectory())
    {
        fileCountLabel.setText("No WAV files found", juce::dontSendNotification);
        if (onDirectoryAnalyzed)
            onDirectoryAnalyzed();
        return;
    }

    fileCountLabel.setText("Scanning folder...", juce::dontSendNotification);

    juce::Component::SafePointer<BatchSplitter> weakThis(this);
    scanner = std::make_unique<FolderScanner>(
        inputDir,
        [weakThis](int scanned, int total)
        {
            if (auto* self = weakThis.getComponent())
                self->onScanProgress(scanned, total);
        },
        [weakThis](std::vector<FolderScanner::Entry> entries, int skipped)
        {
            if (auto* self = weakThis.getComponent())
                self->onScanFinished(std::move(entries), skipped);
        });

    // Split must wait for the scan; tell the window now that it is running.
    if (onDirectoryAnalyzed)
        onDirectoryAnalyzed();
}

void BatchSplitter::onScanProgress(int scanned, int total)
{
    fileCountLabel.setText("Scanning folder... " + juce::String(scanned) + " of " + juce::String(total) + " files",
                           juce::dontSendNotification);
}

void BatchSplitter::onScanFinished(std::vector<FolderScanner::Entry> entries, int skipped)
{
    // Called from the scanner's own queued message; the scanner has nothing left to do, so
    // release it (this joins its already-finished thread).
    scannedFiles = std::move(entries);
    fileCount = (int) scannedFiles.size();
    scanner.reset();

    juce::String countText;
    if (fileCount == 0)
        countText = "No WAV files found";
    else if (fileCount == 1)
        countText = "1 WAV file found";
    else
        countText = juce::String(fileCount) + " WAV files found";

    if (skipped > 0)
        countText += " (" + juce::String(skipped) + " unreadable, skipped)";

    fileCountLabel.setText(countText, juce::dontSendNotification);
    juce::Logger::writeToLog("File count updated: " + countText);

    // Notify main component that directory analysis is complete
    if (onDirectoryAnalyzed)
        onDirectoryAnalyzed();
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
        message = "Batch processing cancelled.\n\n";
        message += "Finished before the cancel: " + juce::String(result.successfulFiles) + " of "
                 + juce::String(result.totalFiles) + " files\n";
        message += "Not processed: " + juce::String(result.cancelledFiles) + "\n";
        message += "Files that were still being written were discarded; no existing file was replaced by a partial one.\n";
    }
    else
    {
        message = "Batch processing completed!\n\n";
        message += "Total files: " + juce::String(result.totalFiles) + "\n";
        message += "Successful: " + juce::String(result.successfulFiles) + "\n";
    }

    message += "Failed: " + juce::String(result.failedFiles) + "\n";
    message += "Total time: " + juce::String(result.totalProcessingTimeSeconds, 1) + " seconds\n";

    if (result.failedFiles > 0)
    {
        message += "\nFailed files:\n";
        for (const auto& error : result.errorMessages)
            message += "  " + error + "\n";
    }

    auto messageType = (result.wasCancelled || result.failedFiles > 0) ? juce::MessageBoxIconType::WarningIcon
                                                                        : juce::MessageBoxIconType::InfoIcon;

    auto options = juce::MessageBoxOptions::makeOptionsOk(messageType,
                                                         "Batch Processing Complete",
                                                         message);
    UserNotice::show(options);

    // Leave batch mode: hides the batch labels and Cancel button
    if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
    {
        if (auto* progressPanel = mainComponent->getProgressPanel())
            progressPanel->resetProgress();
    }

    juce::Logger::writeToLog("Batch processing complete: " + juce::String(result.successfulFiles) +
                            " succeeded, " + juce::String(result.failedFiles) + " failed, " +
                            juce::String(result.cancelledFiles) + " cancelled");

    // Tear the run down (pool drained first, then jobs, then manager) and let the window
    // re-enable Split.
    batchRun.reset();

    if (onBusyChanged)
        onBusyChanged();
}
