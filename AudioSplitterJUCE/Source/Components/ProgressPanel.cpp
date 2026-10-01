#include "ProgressPanel.h"
#include "../UI/ModernLookAndFeel.h"

//==============================================================================
// Constants for layout and timing
namespace
{
    constexpr int LABEL_HEIGHT = 24;
    constexpr int PROGRESS_BAR_HEIGHT = 30;
    constexpr int CANCEL_BUTTON_WIDTH = 90;
    constexpr int UPDATE_INTERVAL_MS = 100;
    constexpr double MIN_ELAPSED_SECONDS = 0.001;  // 1ms minimum to prevent division by zero
}

//==============================================================================
ProgressPanel::ProgressPanel()
    : currentProgress(0.0),
      progressBar(currentProgress),
      progressLabel("progressLabel", "Ready")
{
    // Setup progress bar - mirrors ttk.Progressbar (lines 1708-1714). backgroundColourId /
    // foregroundColourId are no longer read: ModernLookAndFeel::drawProgressBar now draws a
    // fixed phosphor-screen + accent-fill treatment (house tokens), so those two per-instance
    // calls were dead code and have been removed.
    progressBar.setTitle("Progress");
    progressBar.setDescription("Shows how much of the current split or batch operation is complete.");
    addAndMakeVisible(progressBar);
    
    // Setup progress label - mirrors progress_label (lines 1703-1707)
    progressLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textPrimary);
    progressLabel.setColour(juce::Label::backgroundColourId, ModernLookAndFeel::Colors::background);
    progressLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(progressLabel);

    // Setup enhanced progress labels (Phase 2.2)
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setColour(juce::Label::textColourId, ModernLookAndFeel::Colors::textSecondary);
        label.setColour(juce::Label::backgroundColourId, ModernLookAndFeel::Colors::background);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setText(text, juce::dontSendNotification);
        label.setVisible(false);  // Hidden by default, shown in batch mode
        addAndMakeVisible(label);
    };

    setupLabel(filesLabel, "");
    setupLabel(currentFileLabel, "");
    setupLabel(timeInfoLabel, "");
    setupLabel(speedLabel, "");

    // Setup cancel button (hidden by default, shown during batch processing). No bespoke colour:
    // zqsfx::ui::LookAndFeel::drawButtonBackground/drawButtonText draw every TextButton the same
    // way regardless of buttonColourId/textColourOffId (house tokens only, never a per-instance
    // colour -- see ModernLookAndFeel.cpp), so the previous red buttonColourId override was dead
    // code and has been removed; the button's own "Cancel" label is the signal, matching every
    // other ZQ SFX product's plain-text cancel/clear controls.
    cancelButton.setButtonText("Cancel");
    cancelButton.setTooltip("Cancel the current operation");
    cancelButton.setTitle("Cancel");
    cancelButton.setDescription("Cancel the current split or batch operation");
    cancelButton.setVisible(false);
    cancelButton.onClick = [this]()
    {
        if (onCancelRequested)
            onCancelRequested();
    };
    addAndMakeVisible(cancelButton);

    // Initialize state
    resetProgress();

    juce::Logger::writeToLog("ProgressPanel initialized");
}

//==============================================================================
ProgressPanel::~ProgressPanel()
{
    stopTimer();
}

//==============================================================================
void ProgressPanel::paint(juce::Graphics& g)
{
    // Use modern background color
    g.fillAll(ModernLookAndFeel::Colors::background);
}

//==============================================================================
int ProgressPanel::getPreferredHeight() const
{
    const int margin = ModernLookAndFeel::Spacing::sm;
    const int spacing = ModernLookAndFeel::Spacing::xs;

    // Batch: files/current-file row, elapsed/speed row, then the bar with Cancel.
    // Simple: status line, then the bar (with Cancel while a single file runs).
    const int labelRows = isBatchMode ? 2 : 1;
    return margin * 2 + labelRows * LABEL_HEIGHT + labelRows * spacing + PROGRESS_BAR_HEIGHT;
}

//==============================================================================
void ProgressPanel::resized()
{
    auto bounds = getLocalBounds();
    const int margin = ModernLookAndFeel::Spacing::sm;  // 8px modern spacing
    const int spacing = ModernLookAndFeel::Spacing::xs; // 4px spacing

    bounds.reduce(margin, margin);

    // Bar row, with the Cancel button at its right end whenever it is showing.
    auto layoutBarRow = [&](juce::Rectangle<int> row)
    {
        if (cancelButton.isVisible())
        {
            cancelButton.setBounds(row.removeFromRight(CANCEL_BUTTON_WIDTH));
            row.removeFromRight(spacing * 2);
        }
        progressBar.setBounds(row);
    };

    if (isBatchMode)
    {
        // Row 1: "Processing: 47 of 200 (23%)" | "Current: <file>"
        auto row1 = bounds.removeFromTop(LABEL_HEIGHT);
        filesLabel.setBounds(row1.removeFromLeft(row1.getWidth() * 2 / 5));
        currentFileLabel.setBounds(row1);
        bounds.removeFromTop(spacing);

        // Row 2: "Elapsed ... | Remaining ..." | "Speed: ..."
        auto row2 = bounds.removeFromTop(LABEL_HEIGHT);
        timeInfoLabel.setBounds(row2.removeFromLeft(row2.getWidth() * 3 / 5));
        speedLabel.setBounds(row2);
        bounds.removeFromTop(spacing);

        layoutBarRow(bounds.removeFromTop(PROGRESS_BAR_HEIGHT));
    }
    else
    {
        progressLabel.setBounds(bounds.removeFromTop(LABEL_HEIGHT));
        bounds.removeFromTop(spacing);

        layoutBarRow(bounds.removeFromTop(PROGRESS_BAR_HEIGHT));
    }
}

//==============================================================================
void ProgressPanel::timerCallback()
{
    // Handle thread-safe progress updates
    juce::ScopedLock lock(progressLock);
    
    if (hasPendingUpdate)
    {
        currentProgress = pendingProgress.load();
        currentText = pendingText;
        hasPendingUpdate = false;
        
        updateProgressDisplay();
    }
}

//==============================================================================
// Progress management - mirrors Python progress bar functions
void ProgressPanel::setProgress(double progress)
{
    currentProgress = juce::jlimit(0.0, 1.0, progress);
    updateProgressDisplay();
}

void ProgressPanel::setProgressText(const juce::String& text)
{
    currentText = text;
    progressLabel.setText(text, juce::dontSendNotification);
}

void ProgressPanel::startProgress()
{
    // mirrors Python progress initialization
    isActive = true;
    cancelPending = false;
    cancelButton.setEnabled(true);
    currentProgress = 0.0;
    currentText = "Starting...";

    updateProgressDisplay();
    startTimer(UPDATE_INTERVAL_MS); // Update every 100ms for smooth progress

    juce::Logger::writeToLog("Progress started");
}

void ProgressPanel::completeProgress()
{
    // mirrors Python progress completion
    currentProgress = 1.0;
    currentText = "Complete";
    isActive = false;
    
    updateProgressDisplay();
    stopTimer();
    
    juce::Logger::writeToLog("Progress completed");
}

void ProgressPanel::resetProgress()
{
    // mirrors Python progress reset
    currentProgress = 0.0;
    currentText = "Ready";
    isActive = false;
    isIndeterminate = false;

    // Reset batch mode
    isBatchMode = false;
    batchCompletedFiles = 0;
    batchTotalFiles = 0;
    batchCurrentFile.clear();

    // Hide batch labels and cancel button, show simple label
    filesLabel.setVisible(false);
    currentFileLabel.setVisible(false);
    timeInfoLabel.setVisible(false);
    speedLabel.setVisible(false);
    cancelButton.setVisible(false);
    cancelButton.setEnabled(true);
    cancelPending = false;
    progressLabel.setVisible(true);

    updateProgressDisplay();
    stopTimer();
    resized();  // Re-layout for simple mode
    if (onPreferredHeightChanged)
        onPreferredHeightChanged();
}

void ProgressPanel::setSimpleModeCancelVisible(bool visible)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!isBatchMode)
    {
        cancelButton.setVisible(visible);
        resized();
        if (onPreferredHeightChanged)
            onPreferredHeightChanged();
    }
}

void ProgressPanel::setCancelPending()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    cancelPending = true;
    cancelButton.setEnabled(false);
    progressLabel.setText("Cancelling...", juce::dontSendNotification);
    currentFileLabel.setText("Cancelling... finishing the files in progress", juce::dontSendNotification);
}

void ProgressPanel::setIndeterminate(bool indeterminate)
{
    // mirrors Python progress indeterminate mode
    isIndeterminate = indeterminate;
    
    if (indeterminate)
    {
        currentProgress = -1.0; // JUCE uses -1 for indeterminate
        currentText = "Processing...";
    }
    else
    {
        currentProgress = 0.0;
    }
    
    updateProgressDisplay();
}

//==============================================================================
// Thread-safe progress updates (for audio processing threads)
void ProgressPanel::updateProgressSafely(double progress, const juce::String& text)
{
    juce::ScopedLock lock(progressLock);
    
    pendingProgress = juce::jlimit(0.0, 1.0, progress);
    pendingText = text;
    hasPendingUpdate = true;
    
    // Timer will pick up the update on the message thread
}

//==============================================================================
// Helper methods
void ProgressPanel::updateProgressDisplay()
{
    // Update progress bar value
    if (isIndeterminate)
    {
        // For indeterminate mode, we could animate or use a different visual
        progressBar.setPercentageDisplay(false);
    }
    else
    {
        progressBar.setPercentageDisplay(true);
        
        // Update the progress value (JUCE ProgressBar automatically reads from the variable)
        repaint(); // Trigger repaint to show updated progress
    }
    
    // Update text
    progressLabel.setText(cancelPending ? juce::String("Cancelling...") : currentText, juce::dontSendNotification);

    juce::Logger::writeToLog("Progress updated: " + juce::String(currentProgress * 100.0, 1) +
                            "% - " + currentText);
}

//==============================================================================
// Enhanced batch progress reporting (Phase 2.2)

void ProgressPanel::updateBatchProgress(double overallProgress,
                                       const juce::String& currentFile,
                                       int completedCount,
                                       int totalCount)
{
    // Validate inputs (defensive programming)
    jassert(completedCount >= 0 && totalCount >= 0);
    jassert(completedCount <= totalCount);
    overallProgress = juce::jlimit(0.0, 1.0, overallProgress);
    completedCount = juce::jmax(0, completedCount);
    totalCount = juce::jmax(0, totalCount);

    // Thread-safe state update
    {
        juce::ScopedLock lock(progressLock);
        currentProgress = overallProgress;
        batchCompletedFiles = juce::jmin(completedCount, totalCount);  // Ensure <= totalCount
        batchTotalFiles = totalCount;
        batchCurrentFile = currentFile;

        // Enable batch mode if not already
        if (!isBatchMode && totalCount > 0)
        {
            isBatchMode = true;
            batchStartTime = juce::Time::getCurrentTime();
        }
    }

    // Update UI (must be on message thread)
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Show batch-specific labels if transitioning to batch mode
    if (isBatchMode && !filesLabel.isVisible())
    {
        filesLabel.setVisible(true);
        currentFileLabel.setVisible(true);
        timeInfoLabel.setVisible(true);
        speedLabel.setVisible(true);
        cancelButton.setVisible(true);
        progressLabel.setVisible(false);
        resized();  // Re-layout UI for batch mode
        if (onPreferredHeightChanged)
            onPreferredHeightChanged();
    }

    // Update batch display
    updateBatchDisplay();

    // Trigger UI update
    repaint();
}

void ProgressPanel::updateBatchDisplay()
{
    // Thread-safe read of batch state
    int completed, total;
    juce::String currentFile;
    juce::Time startTime;
    {
        juce::ScopedLock lock(progressLock);
        completed = batchCompletedFiles;
        total = batchTotalFiles;
        currentFile = batchCurrentFile;
        startTime = batchStartTime;
    }

    // Update files progress: "Processing: 47 of 200 (23%)"
    int percentage = (total > 0) ?
        juce::jlimit(0, 100, static_cast<int>((completed * 100.0) / total)) : 0;

    // Use String::formatted for better performance
    filesLabel.setText(juce::String::formatted("Processing: %d of %d (%d%%)",
                      completed, total, percentage),
                      juce::dontSendNotification);

    // Update current file: "Current: AMBNat_Ocean_Waves_48kHz.wav"
    if (cancelPending)
    {
        currentFileLabel.setText("Cancelling... finishing the files in progress", juce::dontSendNotification);
    }
    else if (currentFile.isNotEmpty())
    {
        currentFileLabel.setText("Current: " + currentFile, juce::dontSendNotification);
    }
    else
    {
        currentFileLabel.setText("Current: —", juce::dontSendNotification);
    }

    // Calculate time elapsed (safe, minimum threshold to prevent division by zero)
    double elapsedSeconds = juce::jmax(MIN_ELAPSED_SECONDS,
        (juce::Time::getCurrentTime() - startTime).inSeconds());

    // Calculate time remaining (estimate based on average speed)
    juce::String remainingStr;
    if (completed > 0 && completed < total)
    {
        // Safe division - protected by MIN_ELAPSED_SECONDS
        double avgSecondsPerFile = elapsedSeconds / static_cast<double>(completed);
        int remainingFiles = total - completed;
        int estimatedRemainingSeconds = static_cast<int>(avgSecondsPerFile * remainingFiles);
        remainingStr = "~" + formatTime(estimatedRemainingSeconds);
    }
    else if (completed >= total)
    {
        remainingStr = "Complete";
    }
    else
    {
        remainingStr = "Calculating...";
    }

    // Update time info: "Elapsed: 2:15 | Remaining: ~8:30"
    timeInfoLabel.setText("Elapsed: " + formatTime(static_cast<int>(elapsedSeconds)) +
                         " | Remaining: " + remainingStr,
                         juce::dontSendNotification);

    // Calculate processing speed (safe division)
    juce::String speedStr;
    if (elapsedSeconds > MIN_ELAPSED_SECONDS && completed > 0)
    {
        double filesPerMin = (static_cast<double>(completed) / elapsedSeconds) * 60.0;
        speedStr = formatSpeed(filesPerMin);
    }
    else
    {
        speedStr = "—";
    }

    // Update speed: "Speed: 3.2 files/min"
    speedLabel.setText("Speed: " + speedStr, juce::dontSendNotification);
}

juce::String ProgressPanel::formatTime(int seconds) const
{
    if (seconds < 0)
    {
        // Negative time indicates a bug - log warning
        jassertfalse;  // Catch in debug builds
        juce::Logger::writeToLog("WARNING: Negative time value in formatTime(): " +
                                juce::String(seconds));
        return "0:00";
    }

    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0)
    {
        // Format as HH:MM:SS
        return juce::String::formatted("%d:%02d:%02d", hours, minutes, secs);
    }
    else
    {
        // Format as MM:SS
        return juce::String::formatted("%d:%02d", minutes, secs);
    }
}

juce::String ProgressPanel::formatSpeed(double filesPerMin) const
{
    if (filesPerMin < 0.01)
        return "—";

    // Format with 1 decimal place
    return juce::String(filesPerMin, 1) + " files/min";
}