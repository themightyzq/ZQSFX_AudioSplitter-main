#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Progress Panel Component - mirrors Python progress_bar setup
 * 
 * Python equivalent: Lines 1703-1716 (Progress Bar setup)
 * Responsible for:
 * - Progress bar display (mirrors progress_bar widget)
 * - Progress text/status updates (mirrors progress_label)
 * - File processing progress tracking
 * - Thread-safe progress updates
 */
class ProgressPanel : public juce::Component,
                     public juce::Timer
{
public:
    //==============================================================================
    ProgressPanel();
    ~ProgressPanel() override;

    //==============================================================================
    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // Timer interface for progress updates
    void timerCallback() override;

    //==============================================================================
    // Progress management - mirrors Python progress bar functions
    
    /**
     * Set progress value (0.0 to 1.0) - mirrors Python progress_bar['value'] updates
     */
    void setProgress(double progress);
    
    /**
     * Set progress text - mirrors Python progress_label text updates
     */
    void setProgressText(const juce::String& text);
    
    /**
     * Start progress tracking - mirrors Python progress initialization
     */
    void startProgress();
    
    /**
     * Complete progress - mirrors Python progress completion
     */
    void completeProgress();
    
    /**
     * Reset progress - mirrors Python progress reset
     */
    void resetProgress();
    
    /**
     * Set indeterminate mode - mirrors Python progress indeterminate mode
     */
    void setIndeterminate(bool indeterminate);

    /**
     * Show or hide cancel button in simple (non-batch) mode
     */
    void setSimpleModeCancelVisible(bool visible);

    //==============================================================================
    // Enhanced batch progress reporting (Phase 2.2)

    /**
     * Update batch progress with comprehensive information
     * @param overallProgress Overall progress 0.0 to 1.0
     * @param currentFile Name of file currently being processed
     * @param completedCount Number of files completed
     * @param totalCount Total number of files
     */
    void updateBatchProgress(double overallProgress,
                            const juce::String& currentFile,
                            int completedCount,
                            int totalCount);

    //==============================================================================
    // Thread-safe progress updates (for audio processing threads)

    /**
     * Thread-safe progress update - can be called from any thread
     */
    void updateProgressSafely(double progress, const juce::String& text);

    //==============================================================================
    // Cancellation

    /**
     * Callback invoked when user clicks Cancel during batch processing
     */
    std::function<void()> onCancelRequested;

private:
    //==============================================================================
    // UI Components - mirrors Python progress UI structure

    // currentProgress MUST be declared (and therefore constructed) before progressBar: JUCE's
    // juce::ProgressBar binds a reference to it AND reads it immediately to seed its own internal
    // currentValue (juce_ProgressBar.h: `double currentValue { jlimit (0.0, 1.0, progress) };`).
    // Members are constructed in DECLARATION order regardless of member-initializer-list order,
    // so with progressBar declared first (as this was before this fix), that seeding read
    // currentProgress's storage before its own {0.0} initializer had run -- undefined behaviour
    // (a read of an uninitialized double) that happened to render as "0%" in some builds and a
    // stray "100%" filled bar in others. Found while migrating this panel to the house
    // LookAndFeel's drawProgressBar, which made the bug immediately visible; fixed here as a
    // root-cause correctness fix, not a style change.
    double currentProgress {0.0};

    // Progress bar - mirrors ttk.Progressbar (lines 1708-1714)
    juce::ProgressBar progressBar;

    // Progress text label - mirrors progress_label (lines 1703-1707)
    juce::Label progressLabel;

    // Cancel button (visible during batch processing)
    juce::TextButton cancelButton;

    // Enhanced progress info labels (Phase 2.2)
    juce::Label filesLabel;           // "Processing: 47 of 200 (23%)"
    juce::Label currentFileLabel;     // "Current: AMBNat_Ocean_Waves_48kHz.wav"
    juce::Label timeInfoLabel;        // "Elapsed: 2:15 | Remaining: ~8:30"
    juce::Label speedLabel;           // "Speed: 3.2 files/min | 45 MB/s"

    //==============================================================================
    // State management
    juce::String currentText;
    bool isIndeterminate {false};
    bool isActive {false};

    // Batch progress tracking (Phase 2.2)
    int batchCompletedFiles {0};
    int batchTotalFiles {0};
    juce::String batchCurrentFile;
    juce::Time batchStartTime;
    bool isBatchMode {false};

    // Thread-safe updates
    juce::CriticalSection progressLock;
    std::atomic<double> pendingProgress {0.0};
    juce::String pendingText;
    bool hasPendingUpdate {false};

    //==============================================================================
    // Helper methods
    void updateProgressDisplay();       // updates the actual UI components
    void updateBatchDisplay();          // updates batch-specific displays
    juce::String formatTime(int seconds) const;  // formats seconds as MM:SS or HH:MM:SS
    juce::String formatSpeed(double filesPerMin) const;  // formats processing speed

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgressPanel)
};