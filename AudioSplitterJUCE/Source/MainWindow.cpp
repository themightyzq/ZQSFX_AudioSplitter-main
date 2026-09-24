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
    
    // Create main component - DocumentWindow takes ownership
    setContentOwned(new MainComponent(configManager), true);
    
    // Ensure size is correct after setting content
    setSize(defaultWidth, defaultHeight);
    
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
    // Set window size - much larger for proper UI display
    setSize(defaultWidth, defaultHeight);
    
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