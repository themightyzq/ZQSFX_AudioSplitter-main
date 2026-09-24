#include "SingleFileSplitter.h"
#include "../UI/ModernLookAndFeel.h"
#include "../MainComponent.h"

//==============================================================================
SingleFileSplitter::SingleFileSplitter(ConfigManager* config)
    : configManager(config),
      fileLabel("fileLabel", "Select Audio File:"),
      browseFileButton("Browse..."),
      openFileButton("Open Location"),
      outputDirLabel("outputDirLabel", "Output Directory:"),
      browseOutputButton("Browse..."),
      openOutputButton("Open Location"),
      channelGroup("channelGroup", "Channel Selection")
{
    // Setup file selection UI - mirrors Python lines 1327-1367
    fileLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(fileLabel);
    
    filePathEditor.setReadOnly(true);
    filePathEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    filePathEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(filePathEditor);
    
    browseFileButton.onClick = [this]() { browseForFile(); };
    browseFileButton.setTooltip("Browse for an audio file to split");
    browseFileButton.setTitle("Browse for Audio File");
    browseFileButton.setDescription("Browse for an audio file to split");
    addAndMakeVisible(browseFileButton);

    openFileButton.onClick = [this]() { openFileLocation(); };
    openFileButton.setTooltip("Open file location in file browser");
    openFileButton.setTitle("Open File Location");
    openFileButton.setDescription("Open file location in file browser");
    addAndMakeVisible(openFileButton);
    
    // Setup output directory UI - mirrors Python lines 1369-1409
    outputDirLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(outputDirLabel);
    
    outputDirEditor.setReadOnly(true);
    outputDirEditor.setColour(juce::TextEditor::backgroundColourId, ModernLookAndFeel::Colors::surface);
    outputDirEditor.setColour(juce::TextEditor::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(outputDirEditor);
    
    browseOutputButton.onClick = [this]() { browseForOutputDir(); };
    browseOutputButton.setTooltip("Browse for output directory");
    browseOutputButton.setTitle("Browse for Output Directory");
    browseOutputButton.setDescription("Browse for output directory");
    addAndMakeVisible(browseOutputButton);

    openOutputButton.onClick = [this]() { openOutputLocation(); };
    openOutputButton.setTooltip("Open output directory in file browser");
    openOutputButton.setTitle("Open Output Location");
    openOutputButton.setDescription("Open output directory in file browser");
    addAndMakeVisible(openOutputButton);

    // Setup channel selection - mirrors Python lines 1411-1453
    // (GroupComponent::textColourId is no longer read: ModernLookAndFeel::drawGroupComponentOutline
    // now draws the title in the house's fixed silkTitle colour, matching zqsfx::ui::Panel.)
    addAndMakeVisible(channelGroup);

    channelSelectionLabel.setText("Select channels to extract:", juce::dontSendNotification);
    channelSelectionLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    addAndMakeVisible(channelSelectionLabel);

    // Setup channel selection helper buttons (Phase 2 UI/UX improvement)
    selectAllButton.setButtonText("All");
    selectAllButton.setTooltip("Select all channels");
    selectAllButton.setTitle("Select All Channels");
    selectAllButton.setDescription("Select all channels");
    selectAllButton.onClick = [this]() { selectAllChannels(); };
    addAndMakeVisible(selectAllButton);

    selectNoneButton.setButtonText("None");
    selectNoneButton.setTooltip("Deselect all channels");
    selectNoneButton.setTitle("Deselect All Channels");
    selectNoneButton.setDescription("Deselect all channels");
    selectNoneButton.onClick = [this]() { selectNoChannels(); };
    addAndMakeVisible(selectNoneButton);

    invertSelectionButton.setButtonText("Invert");
    invertSelectionButton.setTooltip("Invert channel selection");
    invertSelectionButton.setTitle("Invert Channel Selection");
    invertSelectionButton.setDescription("Invert channel selection");
    invertSelectionButton.onClick = [this]() { invertChannelSelection(); };
    addAndMakeVisible(invertSelectionButton);

    // Channel count label
    channelCountLabel.setText("0 of 0 channels selected", juce::dontSendNotification);
    channelCountLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
    channelCountLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(channelCountLabel);

    // Initialize audio processor for background processing
    audioProcessor = std::make_unique<AudioFileProcessor>();
    
    // Initialize with config defaults
    if (configManager)
    {
        currentOutputDir = configManager->getLastOutputDir();
        outputDirEditor.setText(currentOutputDir);
    }
    
    juce::Logger::writeToLog("SingleFileSplitter initialized");
}

//==============================================================================
SingleFileSplitter::~SingleFileSplitter()
{
}

//==============================================================================
void SingleFileSplitter::paint(juce::Graphics& g)
{
    // Use modern background color
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
void SingleFileSplitter::resized()
{
    auto bounds = getLocalBounds();
    const int margin = ModernLookAndFeel::Spacing::md;     // 16px
    const int buttonWidth = 120;                           // Increased from 100 for better readability
    const int minRowHeight = 40;                           // Increased from 30 for better touch targets
    const int spacing = ModernLookAndFeel::Spacing::sm;    // 8px
    
    // Calculate responsive row height based on available space
    int totalHeight = bounds.getHeight();
    int rowHeight = juce::jmax(minRowHeight, totalHeight / 15); // At least 1/15 of total height
    
    bounds.reduce(margin, margin);
    
    // File selection row - improved responsive layout
    auto fileRow = bounds.removeFromTop(rowHeight);
    int labelWidth = juce::jmax(140, bounds.getWidth() / 6); // Responsive label width, minimum 140px
    fileLabel.setBounds(fileRow.removeFromLeft(labelWidth));
    openFileButton.setBounds(fileRow.removeFromRight(buttonWidth));
    fileRow.removeFromRight(spacing);
    browseFileButton.setBounds(fileRow.removeFromRight(buttonWidth));
    fileRow.removeFromRight(spacing);
    filePathEditor.setBounds(fileRow);
    
    bounds.removeFromTop(spacing);
    
    // Output directory row - improved responsive layout
    auto outputRow = bounds.removeFromTop(rowHeight);
    outputDirLabel.setBounds(outputRow.removeFromLeft(labelWidth)); // Use same responsive width
    openOutputButton.setBounds(outputRow.removeFromRight(buttonWidth));
    outputRow.removeFromRight(spacing);
    browseOutputButton.setBounds(outputRow.removeFromRight(buttonWidth));
    outputRow.removeFromRight(spacing);
    outputDirEditor.setBounds(outputRow);
    
    bounds.removeFromTop(spacing * 2);
    
    // Channel selection group - takes remaining space
    channelGroup.setBounds(bounds);
    
    auto channelBounds = bounds.reduced(margin);
    channelBounds.removeFromTop(20); // group header space
    channelSelectionLabel.setBounds(channelBounds.removeFromTop(rowHeight));

    // Channel selection helper buttons row (Phase 2 UI/UX improvement)
    auto helperRow = channelBounds.removeFromTop(rowHeight);

    // Left side: helper buttons
    selectAllButton.setBounds(helperRow.removeFromLeft(kHelperButtonWidth));
    helperRow.removeFromLeft(spacing);
    selectNoneButton.setBounds(helperRow.removeFromLeft(kHelperButtonWidth));
    helperRow.removeFromLeft(spacing);
    invertSelectionButton.setBounds(helperRow.removeFromLeft(kHelperButtonWidth));

    // Right side: channel count label
    channelCountLabel.setBounds(helperRow);

    channelBounds.removeFromTop(spacing); // spacing before channel grid

    // Layout channel buttons in a responsive grid
    if (channelButtons.size() > 0)
    {
        // Calculate responsive button layout
        int availableWidth = channelBounds.getWidth();
        int minButtonWidth = 120;  // Minimum width for readability
        int buttonsPerRow = juce::jmax(1, availableWidth / (minButtonWidth + spacing));
        int buttonWidth = (availableWidth - (buttonsPerRow - 1) * spacing) / buttonsPerRow;
        int buttonHeight = juce::jmax(35, rowHeight); // Increased minimum height from 25 to 35
        
        for (int i = 0; i < channelButtons.size(); ++i)
        {
            int row = i / buttonsPerRow;
            int col = i % buttonsPerRow;
            
            int x = col * (buttonWidth + spacing);
            int y = row * (buttonHeight + spacing);
            
            channelButtons[i]->setBounds(x, y, buttonWidth, buttonHeight);
        }
    }
}

//==============================================================================
// FileDragAndDropTarget interface - mirrors Python drag-and-drop (lines 1344-1350)
bool SingleFileSplitter::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Accept single audio files
    return files.size() == 1 && files[0].toLowerCase().endsWithIgnoreCase(".wav");
}

void SingleFileSplitter::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    repaint(); // Visual feedback
}

void SingleFileSplitter::fileDragExit(const juce::StringArray& /*files*/)
{
    repaint(); // Remove visual feedback
}

void SingleFileSplitter::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.size() > 0)
    {
        currentFilePath = files[0];
        filePathEditor.setText(currentFilePath);

        // SMART OUTPUT DIRECTORY: Auto-create output folder based on input filename
        auto inputFile = juce::File(currentFilePath);
        auto inputDir = inputFile.getParentDirectory();
        auto baseName = inputFile.getFileNameWithoutExtension();

        // Create smart output directory: input_directory/filename_split/
        auto smartOutputDir = inputDir.getChildFile(baseName + "_split");

        // Create directory if it doesn't exist
        if (!smartOutputDir.exists())
        {
            auto result = smartOutputDir.createDirectory();
            if (result.wasOk())
            {
                juce::Logger::writeToLog("Created output directory: " + smartOutputDir.getFullPathName());
            }
            else
            {
                juce::Logger::writeToLog("Warning: Could not create output directory: " + result.getErrorMessage());
            }
        }

        // Set as current output directory
        currentOutputDir = smartOutputDir.getFullPathName();
        outputDirEditor.setText(currentOutputDir);

        // Update config with new directories
        if (configManager)
        {
            configManager->setLastInputDir(inputDir.getFullPathName());
            configManager->setLastOutputDir(currentOutputDir);
        }

        // Analyze file and update channel buttons - mirrors Python drag-and-drop logic
        updateChannelButtons();

        juce::Logger::writeToLog("File dropped: " + currentFilePath);
        juce::Logger::writeToLog("Auto-selected output: " + currentOutputDir);

        // Notify parent to update button states
        if (auto* mainComp = findParentComponentOfClass<MainComponent>())
        {
            mainComp->updateButtonStates();
        }
    }
}

//==============================================================================
juce::String SingleFileSplitter::getSelectedFile() const
{
    return currentFilePath;
}

juce::String SingleFileSplitter::getOutputDirectory() const
{
    return currentOutputDir;
}

void SingleFileSplitter::startProcessing()
{
    // Validate inputs before starting processing - mirrors Python validation (lines 673-685)
    if (currentFilePath.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "Invalid Input",
                                                             "Please select an audio file to process.");
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
    
    auto selectedChannels = getSelectedChannels();
    if (selectedChannels.empty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "No Channels Selected",
                                                             "Please select at least one channel to extract.");
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }
    
    // Check if already processing
    if (audioProcessor->isProcessing())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::InfoIcon,
                                                             "Processing In Progress",
                                                             "Audio processing is already in progress.");
        juce::AlertWindow::showAsync(options, nullptr);
        return;
    }
    
    // Build processing options from UI state
    AudioFileProcessor::ProcessingOptions options;
    options.inputFilePath = currentFilePath;
    options.outputDirectory = currentOutputDir;
    options.selectedChannels = selectedChannels;
    
    // Get settings from OptionsPanel - mirrors Python options integration
    if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
    {
        if (auto* optionsPanel = mainComponent->getOptionsPanel())
        {
            // Get custom channel names
            options.customChannelNames = optionsPanel->getCustomNames();
            
            // Parse sample rate override - mirrors Python lines 1752-1756
            if (optionsPanel->getOverrideSampleRate())
            {
                auto sampleRateStr = optionsPanel->getSampleRate();
                if (sampleRateStr.contains("Hz"))
                {
                    // Extract number from string like "48000 Hz"
                    options.sampleRate = sampleRateStr.getDoubleValue();
                }
                else if (sampleRateStr != "Same as input")
                {
                    options.sampleRate = sampleRateStr.getDoubleValue();
                }
                else
                {
                    options.sampleRate = 0.0; // Keep original
                }
            }
            else
            {
                options.sampleRate = 0.0; // Keep original if override not enabled
            }
            
            // Parse bit depth override - mirrors Python lines 1757-1761
            if (optionsPanel->getOverrideBitDepth())
            {
                auto bitDepthStr = optionsPanel->getBitDepth();
                if (bitDepthStr.contains("bit"))
                {
                    // Extract number from string like "16 bit"
                    options.bitDepth = bitDepthStr.getIntValue();
                }
                else if (bitDepthStr != "Same as input")
                {
                    options.bitDepth = bitDepthStr.getIntValue();
                }
                else
                {
                    options.bitDepth = 0; // Keep original
                }
            }
            else
            {
                options.bitDepth = 0; // Keep original if override not enabled
            }
            
            // Get checkbox options
            options.preserveMetadata = optionsPanel->getPreserveIXML();
            options.stereoToMono = optionsPanel->getStereoToMono();
        }
    }
    else
    {
        // Fallback defaults if OptionsPanel not accessible
        juce::Logger::writeToLog("Warning: Could not access OptionsPanel, using defaults");
        options.customChannelNames = "";
        options.sampleRate = 0.0;
        options.bitDepth = 0;
        options.preserveMetadata = true;
        options.stereoToMono = false;
    }

    // Get UCS naming settings from MainComponent
    if (auto* mainComp = findParentComponentOfClass<MainComponent>())
    {
        if (auto* ucsPanel = mainComp->getUCSNamingPanel())
        {
            options.useUCSNaming = ucsPanel->isUCSEnabled();
            if (options.useUCSNaming)
            {
                options.ucsCategory = ucsPanel->getCategory();
                options.ucsSubcategory = ucsPanel->getSubcategory();
                options.ucsDescription = ucsPanel->getDescription();

                // Generate standard channel suffixes using UCSManager
                if (auto* ucsManager = mainComp->getUCSManager())
                {
                    options.ucsChannelSuffixes = ucsManager->getChannelSuffixes(selectedChannels.size());
                }

                juce::Logger::writeToLog("UCS naming enabled: " + options.ucsCategory + "_" +
                                         options.ucsSubcategory + "_" + options.ucsDescription);
            }
        }
    }

    juce::Logger::writeToLog("Starting audio processing with " + juce::String(selectedChannels.size()) +
                            " channels from: " + currentFilePath);
    
    // Start processing with progress callbacks
    audioProcessor->startProcessing(options,
                                   [this](double progress, const juce::String& message)
                                   {
                                       onProcessingProgress(progress, message);
                                   },
                                   [this](const AudioFileProcessor::ProcessingResult& result)
                                   {
                                       onProcessingComplete(result);
                                   });
}

//==============================================================================
// Helper methods - mirror Python helper functions
void SingleFileSplitter::updateChannelButtons()
{
    // mirrors update_channel_checkboxes() (lines 1016-1048)
    juce::Logger::writeToLog("update_channel_checkboxes called with file_path: " + currentFilePath);
    
    // Clear existing channel buttons
    channelButtons.clear();
    
    // Validate file path - mirrors Python checks (lines 1020-1022)
    if (currentFilePath.isEmpty() || !audioAnalyzer.fileExists(currentFilePath))
    {
        juce::Logger::writeToLog("File path is invalid or does not exist.");
        detectedChannels = 0;
        repaint(); // Update UI
        return;
    }
    
    // Analyze the audio file - mirrors Python FFprobe command (lines 1024-1035)
    currentFileInfo = audioAnalyzer.analyzeFile(currentFilePath);
    
    if (!currentFileInfo.isValid)
    {
        juce::Logger::writeToLog("Error analyzing file: " + currentFileInfo.errorMessage);
        detectedChannels = 0;
        repaint(); // Update UI
        return;
    }
    
    detectedChannels = currentFileInfo.numChannels;
    juce::Logger::writeToLog("Number of channels from audio file: " + juce::String(detectedChannels));
    
    // Create channel selection buttons - mirrors Python checkbox creation (lines 1037-1045)
    const int maxChannels = 16; // Reasonable maximum for UI
    int channelsToShow = juce::jmin(maxChannels, juce::jmax(detectedChannels, 8)); // Show at least 8
    
    for (int i = 0; i < channelsToShow; ++i)
    {
        auto* channelButton = new juce::ToggleButton("Channel " + juce::String(i + 1));
        channelButton->setColour(juce::ToggleButton::textColourId, ModernLookAndFeel::Colors::textPrimary);

        // Accessibility floor: visible label + tooltip + description (style guide section 8).
        const auto channelTip = "Include channel " + juce::String(i + 1) + " in the split output";
        channelButton->setTitle("Channel " + juce::String(i + 1));
        channelButton->setTooltip(channelTip);
        channelButton->setDescription(channelTip);

        // Enable/disable based on detected channels - mirrors Python logic (lines 1038-1045)
        if (i < detectedChannels)
        {
            channelButton->setEnabled(true);
            channelButton->setToggleState(true, juce::dontSendNotification); // Default to selected
            juce::Logger::writeToLog("Enabled checkbox for Channel " + juce::String(i + 1));
        }
        else
        {
            // Disabled channels: clear state and don't add callback to prevent stale state
            channelButton->setEnabled(false);
            channelButton->setToggleState(false, juce::dontSendNotification);
            channelButton->onStateChange = nullptr;  // Medium priority fix: Clear callback for disabled buttons
            juce::Logger::writeToLog("Disabled checkbox for Channel " + juce::String(i + 1));
        }

        // Add onStateChange callback to update channel count label (Phase 2 UI/UX improvement)
        // Only for enabled buttons (disabled buttons have nullptr callback set above)
        if (i < detectedChannels)
        {
            channelButton->onStateChange = [this]() {
                updateChannelCountLabel();
            };
        }

        channelButtons.add(channelButton);
        channelGroup.addAndMakeVisible(channelButton);
    }

    // Update channel count label with initial state (Phase 2 UI/UX improvement)
    updateChannelCountLabel();

    // Trigger layout update
    resized();
    repaint();

    // Notify main component that file analysis is complete - mirrors Python update_button_states() call
    if (onFileAnalyzed)
        onFileAnalyzed();
}

void SingleFileSplitter::browseForFile()
{
    // mirrors browse_single_file() (lines 657-671)
    fileChooser = std::make_unique<juce::FileChooser>("Select Audio File", 
                                                      juce::File(configManager ? configManager->getLastInputDir() : ""),
                                                      "*.wav");
    
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;
    
    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File{})
        {
            currentFilePath = file.getFullPathName();
            filePathEditor.setText(currentFilePath);
            
            if (configManager)
            {
                configManager->setLastInputDir(file.getParentDirectory().getFullPathName());
            }
            
            // Analyze file and update channel buttons - mirrors Python (lines 667-668)
            updateChannelButtons();
            
            juce::Logger::writeToLog("File selected: " + currentFilePath);
        }
    });
}

void SingleFileSplitter::browseForOutputDir()
{
    // mirrors browse_output_dir() (lines 459-472)
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

void SingleFileSplitter::openFileLocation()
{
    // mirrors open_file_directory() (lines 598-610)
    if (!currentFilePath.isEmpty())
    {
        juce::File(currentFilePath).getParentDirectory().revealToUser();
    }
}

void SingleFileSplitter::openOutputLocation()
{
    // mirrors open_output_directory() (lines 429-441)
    if (!currentOutputDir.isEmpty())
    {
        juce::File(currentOutputDir).revealToUser();
    }
}

//==============================================================================
// Channel selection helper methods (Phase 2 UI/UX improvement)

void SingleFileSplitter::selectAllChannels()
{
    // High priority fix: Defensive check for empty channel buttons
    if (channelButtons.isEmpty())
    {
        juce::Logger::writeToLog("Warning: No channel buttons available");
        return;
    }

    // Select all available (enabled) channels
    for (auto* button : channelButtons)
    {
        if (button->isEnabled())
        {
            button->setToggleState(true, juce::sendNotification);
        }
    }
    updateChannelCountLabel();
    juce::Logger::writeToLog("Selected all channels");
}

void SingleFileSplitter::selectNoChannels()
{
    // High priority fix: Defensive check for empty channel buttons
    if (channelButtons.isEmpty())
    {
        juce::Logger::writeToLog("Warning: No channel buttons available");
        return;
    }

    // Deselect all channels
    for (auto* button : channelButtons)
    {
        button->setToggleState(false, juce::sendNotification);
    }
    updateChannelCountLabel();
    juce::Logger::writeToLog("Deselected all channels");
}

void SingleFileSplitter::invertChannelSelection()
{
    // High priority fix: Defensive check for empty channel buttons
    if (channelButtons.isEmpty())
    {
        juce::Logger::writeToLog("Warning: No channel buttons available");
        return;
    }

    // Invert the current selection (only for enabled channels)
    for (auto* button : channelButtons)
    {
        if (button->isEnabled())
        {
            button->setToggleState(!button->getToggleState(), juce::sendNotification);
        }
    }
    updateChannelCountLabel();
    juce::Logger::writeToLog("Inverted channel selection");
}

void SingleFileSplitter::updateChannelCountLabel()
{
    // Count selected and total channels
    int selectedCount = 0;
    int totalCount = 0;

    for (auto* button : channelButtons)
    {
        if (button->isEnabled())
        {
            totalCount++;
            if (button->getToggleState())
            {
                selectedCount++;
            }
        }
    }

    // Update label text: "X of Y channels selected"
    channelCountLabel.setText(
        juce::String(selectedCount) + " of " + juce::String(totalCount) + " channels selected",
        juce::dontSendNotification
    );
}

//==============================================================================
// Audio processing methods
std::vector<int> SingleFileSplitter::getSelectedChannels() const
{
    std::vector<int> selectedChannels;
    
    // Check which channel buttons are selected
    for (int i = 0; i < channelButtons.size(); ++i)
    {
        if (channelButtons[i]->getToggleState())
        {
            selectedChannels.push_back(i); // 0-based channel index
        }
    }
    
    return selectedChannels;
}

void SingleFileSplitter::onProcessingProgress(double progress, const juce::String& message)
{
    // Update progress display - connects to ProgressPanel
    juce::Logger::writeToLog("Progress: " + juce::String(progress * 100.0, 1) + "% - " + message);
    
    // Update ProgressPanel - mirrors Python message_queue.put() pattern
    if (auto* mainComponent = findParentComponentOfClass<MainComponent>())
    {
        if (auto* progressPanel = mainComponent->getProgressPanel())
        {
            // Update progress safely - already thread-safe
            progressPanel->updateProgressSafely(progress, message);
        }
    }
}

void SingleFileSplitter::onProcessingComplete(const AudioFileProcessor::ProcessingResult& result)
{
    if (result.success)
    {
        juce::String message = "Processing completed successfully!\n\n";
        message += "Files created:\n";
        for (const auto& file : result.outputFiles)
        {
            message += "• " + juce::File(file).getFileName() + "\n";
        }
        message += "\nProcessing time: " + juce::String(result.processingTimeSeconds, 2) + " seconds";
        
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::InfoIcon,
                                                             "Processing Complete",
                                                             message);
        juce::AlertWindow::showAsync(options, nullptr);
        
        juce::Logger::writeToLog("Audio processing completed successfully. " + 
                                juce::String(result.outputFiles.size()) + " files created.");
    }
    else
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                             "Processing Failed",
                                                             "Audio processing failed:\n\n" + result.errorMessage);
        juce::AlertWindow::showAsync(options, nullptr);
        
        juce::Logger::writeToLog("Audio processing failed: " + result.errorMessage);
    }
}