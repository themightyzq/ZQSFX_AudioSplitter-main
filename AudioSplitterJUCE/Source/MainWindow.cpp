#include "MainWindow.h"
#include "UI/ModernLookAndFeel.h"

//==============================================================================
MainWindow::MainWindow(const juce::String& name, ConfigManager* config)
    : DocumentWindow(
        name,
        ModernLookAndFeel::Colors::background,
        DocumentWindow::allButtons
      ),
      configManager(config)
{
    // Set up window properties - mirrors Python root window setup (lines 1193-1228)
    setUsingNativeTitleBar(true);
    setDropShadowEnabled(true);
    
    // Set up window sizing FIRST - before creating content
    setupWindow();
    
    // Create main component - DocumentWindow takes ownership. setContentOwned() resizes the
    // window to the content's (empty) size, so put the size chosen in setupWindow() back.
    const int windowWidth = getWidth();
    const int windowHeight = getHeight();
    setContentOwned(new MainComponent(configManager), true);
    setSize(windowWidth, windowHeight);
    
    // Make window visible - mirrors Python root.mainloop() preparation
    setVisible(true);
    
    juce::Logger::writeToLog("Main window initialized successfully");
}

//==============================================================================
MainWindow::~MainWindow()
{
    // Content component is owned by DocumentWindow via setContentOwned
}

//==============================================================================
void MainWindow::closeButtonPressed()
{
    // Mirror Python on_closing() function behavior (lines 639-642)
    juce::Logger::writeToLog("Window close requested - shutting down application");
    
    // This will trigger AudioSplitterApplication::systemRequestedQuit()
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

//==============================================================================
void MainWindow::moved()
{
    DocumentWindow::moved();
    
    // Save window position if needed (future enhancement)
    // Mirrors Python's config persistence philosophy
}

//==============================================================================
void MainWindow::resized()
{
    DocumentWindow::resized();
    
    // The setResizeLimits in setupWindow() handles minimum size enforcement automatically
    // This method can be used for additional responsive behavior if needed
    
    // Optional: Adjust UI scaling based on window size
    if (auto* content = getContentComponent())
    {
        content->resized();
    }
}

//==============================================================================
void MainWindow::setupWindow()
{
    // Open at the default size, but never larger than the screen's usable area (a 13-inch
    // laptop is about 1280x775 below the menu bar).
    int width = defaultWidth;
    int height = defaultHeight;
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        width = juce::jmin(width, display->userArea.getWidth());
        height = juce::jmin(height, display->userArea.getHeight());
    }
    setSize(width, height);

    // Set resize limits - allow window to be resized from minimum to maximum screen size
    setResizeLimits(minWidth, minHeight, 3000, 2000); // Set reasonable maximums
    
    // Enable resizing and maximizing
    setResizable(true, true);
    setFullScreen(false); // Ensure we're not in fullscreen mode
    
    // Center window on screen - mirrors Python centreWithSize approach
    centerWindow();
}

//==============================================================================
void MainWindow::centerWindow()
{
    // Center window on screen - mirrors Python window positioning logic
    auto display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (display != nullptr)
    {
        auto screenBounds = display->userArea;
        auto windowBounds = getBounds();
        
        setBounds(windowBounds.withCentre(screenBounds.getCentre()));
    }
}