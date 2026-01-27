#include "AudioFileProcessor.h"

//==============================================================================
AudioFileProcessor::AudioFileProcessor() : Thread("AudioFileProcessor")
{
    // Register standard audio formats - mirrors Python supported formats
    formatManager.registerBasicFormats();
    
    juce::Logger::writeToLog("AudioFileProcessor initialized");
}

//==============================================================================
AudioFileProcessor::~AudioFileProcessor()
{
    // Signal async callbacks that this object is gone
    aliveFlag->store(false);
    // Ensure thread is stopped
    stopThread(5000); // 5 second timeout
}

//==============================================================================
void AudioFileProcessor::startProcessing(const ProcessingOptions& options,
                                    ProgressCallback progressCallback,
                                    CompletionCallback completionCallback)
{
    // Don't start if already processing
    if (isThreadRunning())
    {
        juce::Logger::writeToLog("AudioFileProcessor: Cannot start - already processing");
        return;
    }

    // Store processing parameters
    currentOptions = options;
    onProgress = progressCallback;
    onCompletion = completionCallback;
    
    // Reset state (thread-safe)
    currentProgress = 0.0;
    shouldCancel = false;
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result = ProcessingResult{};
    }
    
    // Validate input parameters - mirrors Python validation (lines 673-685)
    if (options.inputFilePath.isEmpty())
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.success = false;
        result.errorMessage = "Input file path is empty";
        completeProcessing();
        return;
    }

    if (options.outputDirectory.isEmpty())
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.success = false;
        result.errorMessage = "Output directory is empty";
        completeProcessing();
        return;
    }

    if (options.selectedChannels.empty())
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.success = false;
        result.errorMessage = "No channels selected for extraction";
        completeProcessing();
        return;
    }
    
    juce::Logger::writeToLog("AudioFileProcessor: Starting processing of " + options.inputFilePath);
    
    // Start background thread
    startThread();
}

//==============================================================================
void AudioFileProcessor::cancelProcessing()
{
    juce::Logger::writeToLog("AudioFileProcessor: Cancellation requested");
    shouldCancel = true;
    
    // Stop thread with timeout
    stopThread(3000);
}

//==============================================================================
void AudioFileProcessor::run()
{
    // Record start time for performance measurement
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    
    updateProgress(0.0, "Starting audio processing...");
    
    try
    {
        // Step 1: Load and validate input file (10% of progress)
        updateProgress(0.0, "Loading input file...");
        if (!loadInputFile())
        {
            return; // Error already set
        }
        
        if (threadShouldExit() || shouldCancel.load())
        {
            result.success = false;
            result.errorMessage = "Processing cancelled";
            completeProcessing();
            return;
        }
        
        updateProgress(0.1, "Input file loaded successfully");
        
        // Step 2: Setup output directory (20% of progress)
        updateProgress(0.1, "Setting up output directory...");
        if (!setupOutputDirectory())
        {
            return; // Error already set
        }
        
        updateProgress(0.2, "Output directory ready");
        
        // Step 3: Process channels (20% - 90% of progress)
        updateProgress(0.2, "Processing audio channels...");
        if (!processChannels())
        {
            return; // Error already set
        }
        
        // Step 4: Complete successfully (100%)
        updateProgress(1.0, "Processing completed successfully");
        
        // Calculate processing time
        auto endTime = juce::Time::getMillisecondCounterHiRes();
        result.processingTimeSeconds = (endTime - startTime) / 1000.0;
        result.success = true;
        
        juce::Logger::writeToLog("AudioFileProcessor: Processing completed in " + 
                                juce::String(result.processingTimeSeconds, 2) + " seconds");
    }
    catch (const std::exception& e)
    {
        result.success = false;
        result.errorMessage = "Processing error: " + juce::String(e.what());
        juce::Logger::writeToLog("AudioFileProcessor: Exception caught - " + result.errorMessage);
    }
    catch (...)
    {
        result.success = false;
        result.errorMessage = "Unknown processing error occurred";
        juce::Logger::writeToLog("AudioFileProcessor: Unknown exception caught");
    }
    
    completeProcessing();
}

//==============================================================================
bool AudioFileProcessor::loadInputFile()
{
    // Check if file exists - mirrors Python file validation (lines 686-691)
    juce::File inputFile(currentOptions.inputFilePath);
    if (!inputFile.existsAsFile())
    {
        result.success = false;
        result.errorMessage = "Input file does not exist: " + currentOptions.inputFilePath;
        completeProcessing();
        return false;
    }
    
    // Create audio format reader - mirrors Python audio file loading (lines 693-702)
    auto* reader = formatManager.createReaderFor(inputFile);
    if (reader == nullptr)
    {
        result.success = false;
        result.errorMessage = "Cannot read audio file (unsupported format): " + currentOptions.inputFilePath;
        completeProcessing();
        return false;
    }
    
    audioReader.reset(reader);
    
    // Validate channel selection against actual file
    int actualChannels = (int)audioReader->numChannels;
    for (int channelIndex : currentOptions.selectedChannels)
    {
        if (channelIndex < 0 || channelIndex >= actualChannels)
        {
            result.success = false;
            result.errorMessage = "Invalid channel index " + juce::String(channelIndex + 1) + 
                                 " (file has " + juce::String(actualChannels) + " channels)";
            completeProcessing();
            return false;
        }
    }
    
    juce::Logger::writeToLog("AudioFileProcessor: Loaded " + inputFile.getFileName() + 
                            " (" + juce::String(actualChannels) + " channels, " +
                            juce::String(audioReader->sampleRate, 0) + " Hz, " +
                            juce::String(audioReader->bitsPerSample) + "-bit)");
    
    return true;
}

//==============================================================================
bool AudioFileProcessor::setupOutputDirectory()
{
    // Create output directory if it doesn't exist - mirrors Python directory creation (lines 704-709)
    juce::File outputDir(currentOptions.outputDirectory);
    
    if (!outputDir.exists())
    {
        auto createResult = outputDir.createDirectory();
        if (!createResult.wasOk())
        {
            result.success = false;
            result.errorMessage = "Cannot create output directory: " + createResult.getErrorMessage();
            completeProcessing();
            return false;
        }
    }
    
    if (!outputDir.isDirectory())
    {
        result.success = false;
        result.errorMessage = "Output path is not a directory: " + currentOptions.outputDirectory;
        completeProcessing();
        return false;
    }
    
    return true;
}

//==============================================================================
juce::StringArray AudioFileProcessor::generateOutputFilenames()
{
    juce::StringArray filenames;
    juce::File inputFile(currentOptions.inputFilePath);
    juce::String extension = inputFile.getFileExtension();

    // UCS naming mode
    if (currentOptions.useUCSNaming &&
        currentOptions.ucsCategory.isNotEmpty() &&
        currentOptions.ucsDescription.isNotEmpty())
    {
        // Generate UCS-compliant filenames: Category_Subcategory_Description_Channel.wav
        for (size_t i = 0; i < currentOptions.selectedChannels.size(); ++i)
        {
            int channelIndex = currentOptions.selectedChannels[i];

            // Get channel suffix from provided array or use index
            juce::String channelSuffix;
            if (channelIndex < currentOptions.ucsChannelSuffixes.size())
            {
                channelSuffix = currentOptions.ucsChannelSuffixes[channelIndex];
            }
            else
            {
                channelSuffix = "Ch" + juce::String(channelIndex + 1);
            }

            // Build UCS filename
            juce::String filename = currentOptions.ucsCategory;
            if (currentOptions.ucsSubcategory.isNotEmpty())
                filename += "_" + currentOptions.ucsSubcategory;
            filename += "_" + currentOptions.ucsDescription;
            filename += "_" + channelSuffix;
            filename += extension;

            juce::String fullPath = juce::File(currentOptions.outputDirectory).getChildFile(filename).getFullPathName();
            filenames.add(fullPath);

            juce::Logger::writeToLog("  UCS filename: " + filename);
        }

        return filenames;
    }

    // Standard naming mode (original logic)
    juce::String baseName = inputFile.getFileNameWithoutExtension();

    // Parse custom channel names - mirrors Python custom naming (lines 716-732)
    juce::StringArray customNames;
    if (currentOptions.customChannelNames.isNotEmpty())
    {
        customNames = juce::StringArray::fromTokens(currentOptions.customChannelNames, ",", "");
        // Trim whitespace from each name
        for (int i = 0; i < customNames.size(); ++i)
            customNames.getReference(i) = customNames[i].trim();
    }

    // Generate filename for each selected channel
    for (size_t i = 0; i < currentOptions.selectedChannels.size(); ++i)
    {
        int channelIndex = currentOptions.selectedChannels[i];
        juce::String channelName;

        // Use custom name if provided, otherwise default naming - mirrors Python lines 779-782
        if (channelIndex < customNames.size() && customNames[channelIndex].isNotEmpty())
        {
            channelName = customNames[channelIndex];
        }
        else
        {
            // Default naming - mirrors Python "chan{idx + 1}" format (line 782)
            channelName = "chan" + juce::String(channelIndex + 1); // 1-based for user display
        }

        // Generate output filename: BaseName_ChannelName.ext
        juce::String filename = baseName + "_" + channelName + extension;
        juce::String fullPath = juce::File(currentOptions.outputDirectory).getChildFile(filename).getFullPathName();

        filenames.add(fullPath);
    }

    return filenames;
}

//==============================================================================
bool AudioFileProcessor::processChannels()
{
    auto outputFilenames = generateOutputFilenames();
    double progressPerChannel = 0.7 / currentOptions.selectedChannels.size(); // 70% of total progress for channel processing
    
    result.outputFiles.clear();
    
    // Process each selected channel - mirrors Python channel extraction loop (lines 734-820)
    for (size_t i = 0; i < currentOptions.selectedChannels.size(); ++i)
    {
        if (threadShouldExit() || shouldCancel.load())
        {
            result.success = false;
            result.errorMessage = "Processing cancelled";
            completeProcessing();
            return false;
        }
        
        int channelIndex = currentOptions.selectedChannels[i];
        juce::String outputPath = outputFilenames[i];
        juce::String channelName = juce::File(outputPath).getFileNameWithoutExtension().fromLastOccurrenceOf("_", false, false);
        
        double channelProgressStart = 0.2 + (i * progressPerChannel);
        updateProgress(channelProgressStart, "Processing " + channelName + "...");
        
        if (!extractChannel(channelIndex, outputPath, channelName))
        {
            return false; // Error already set
        }
        
        result.outputFiles.add(outputPath);
        
        double channelProgressEnd = 0.2 + ((i + 1) * progressPerChannel);
        updateProgress(channelProgressEnd, "Completed " + channelName);
        
        juce::Logger::writeToLog("AudioFileProcessor: Extracted channel " + juce::String(channelIndex + 1) + 
                                " to " + juce::File(outputPath).getFileName());
    }
    
    return true;
}

//==============================================================================
bool AudioFileProcessor::extractChannel(int channelIndex, const juce::String& outputPath, const juce::String& channelName)
{
    juce::File outputFile(outputPath);
    
    // Delete existing file if it exists
    if (outputFile.exists())
        outputFile.deleteFile();
    
    // Create audio format writer - mirrors Python audio writer creation
    auto outputStream = outputFile.createOutputStream();
    if (!outputStream)
    {
        result.success = false;
        result.errorMessage = "Cannot create output file: " + outputPath;
        completeProcessing();
        return false;
    }
    
    // Get appropriate audio format (WAV by default)
    auto* format = formatManager.findFormatForFileExtension(outputFile.getFileExtension());
    if (!format)
    {
        result.success = false;
        result.errorMessage = "Unsupported output format: " + outputFile.getFileExtension();
        completeProcessing();
        return false;
    }
    
    // Determine output sample rate and bit depth
    double outputSampleRate = (currentOptions.sampleRate > 0) ? currentOptions.sampleRate : audioReader->sampleRate;
    int outputBitDepth = (currentOptions.bitDepth > 0) ? currentOptions.bitDepth : (int)audioReader->bitsPerSample;

    // Prepare metadata for output file - preserves BWF BEXT, iXML, and all other metadata
    juce::StringPairArray outputMetadata;

    if (currentOptions.preserveMetadata && audioReader->metadataValues.size() > 0)
    {
        // Copy all metadata from source file
        // This includes:
        // - BWF BEXT chunks (Description, Originator, OriginatorReference, OriginationDate,
        //                    OriginationTime, TimeReference/timecode, CodingHistory)
        // - iXML chunks (Scene, Take, Tape, Project, all custom fields)
        // - All other WAV metadata chunks
        outputMetadata = audioReader->metadataValues;

        // Log metadata preservation for verification
        juce::Logger::writeToLog("  Preserving metadata: " + juce::String(outputMetadata.size()) + " fields");

        // Log key metadata fields for the first channel only (avoid spam for multi-channel files)
        if (channelIndex == currentOptions.selectedChannels[0])
        {
            juce::String metadataDescription = outputMetadata.getValue("Description", "(none)");
            juce::String metadataOriginator = outputMetadata.getValue("Originator", "(none)");
            juce::String metadataTimeRef = outputMetadata.getValue("TimeReference", "(none)");

            if (metadataDescription != "(none)" || metadataOriginator != "(none)" || metadataTimeRef != "(none)")
            {
                juce::Logger::writeToLog("  BWF metadata found:");
                juce::Logger::writeToLog("    Description: " + metadataDescription);
                juce::Logger::writeToLog("    Originator: " + metadataOriginator);
                juce::Logger::writeToLog("    TimeReference: " + metadataTimeRef);
            }

            // Check for iXML metadata
            bool hasiXML = false;
            for (int i = 0; i < outputMetadata.size(); ++i)
            {
                if (outputMetadata.getAllKeys()[i].toLowerCase().contains("xml"))
                {
                    hasiXML = true;
                    break;
                }
            }
            if (hasiXML)
            {
                juce::Logger::writeToLog("  iXML metadata found");
            }
        }
    }
    else
    {
        if (audioReader->metadataValues.size() > 0)
        {
            juce::Logger::writeToLog("  Metadata preservation disabled - metadata will not be copied");
        }
    }

    // Create writer with single channel output and preserved metadata
    auto writer = format->createWriterFor(outputStream.release(),
                                         outputSampleRate,
                                         1, // Single channel output
                                         outputBitDepth,
                                         outputMetadata,  // Pass metadata to preserve BWF/iXML
                                         0);
    
    if (!writer)
    {
        result.success = false;
        result.errorMessage = "Cannot create audio writer for: " + outputPath;
        completeProcessing();
        return false;
    }
    
    std::unique_ptr<juce::AudioFormatWriter> writerPtr(writer);
    
    // Check if we need to resample
    bool needsResampling = (outputSampleRate != audioReader->sampleRate) && (currentOptions.sampleRate > 0);
    
    // Process audio in chunks to avoid memory issues and provide progress updates
    const int bufferSize = 8192; // Process in 8K sample chunks
    juce::AudioBuffer<float> readBuffer((int)audioReader->numChannels, bufferSize);
    juce::AudioBuffer<float> writeBuffer(1, bufferSize); // Single channel output
    
    juce::int64 totalSamples = audioReader->lengthInSamples;
    juce::int64 samplesProcessed = 0;
    
    // Set up resampling if needed
    std::unique_ptr<juce::ResamplingAudioSource> resampler;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
    if (needsResampling)
    {
        // Create audio source from reader
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(audioReader.get(), false);
        
        // Create resampler
        double resampleRatio = outputSampleRate / audioReader->sampleRate;
        resampler = std::make_unique<juce::ResamplingAudioSource>(readerSource.get(), false, (int)audioReader->numChannels);
        resampler->setResamplingRatio(resampleRatio);
        
        // Prepare the resampler
        resampler->prepareToPlay(bufferSize, audioReader->sampleRate);
        
        // Adjust total samples for output sample rate
        totalSamples = static_cast<juce::int64>(totalSamples * resampleRatio);
    }
    
    while (samplesProcessed < totalSamples)
    {
        if (threadShouldExit() || shouldCancel.load())
        {
            result.success = false;
            result.errorMessage = "Processing cancelled";
            completeProcessing();
            return false;
        }
        
        // Calculate samples to process this iteration
        int samplesToProcess = juce::jmin(bufferSize, (int)(totalSamples - samplesProcessed));
        
        if (needsResampling)
        {
            // Use resampler for reading
            juce::AudioSourceChannelInfo info(&readBuffer, 0, samplesToProcess);
            resampler->getNextAudioBlock(info);
            
            // Copy selected channel to output buffer
            writeBuffer.copyFrom(0, 0, readBuffer, channelIndex, 0, samplesToProcess);
        }
        else
        {
            // Direct reading from source file
            audioReader->read(&readBuffer, 0, samplesToProcess, samplesProcessed, true, true);
            
            // Copy selected channel to output buffer
            writeBuffer.copyFrom(0, 0, readBuffer, channelIndex, 0, samplesToProcess);
        }
        
        // Write to output file
        writerPtr->writeFromAudioSampleBuffer(writeBuffer, 0, samplesToProcess);
        
        samplesProcessed += samplesToProcess;
        result.totalSamplesProcessed += samplesToProcess;
        
        // Update progress occasionally (every 100K samples to avoid excessive updates)
        if (samplesProcessed % 100000 == 0 || samplesProcessed >= totalSamples)
        {
            double channelProgress = (double)samplesProcessed / totalSamples;
            // This is just for logging - main progress is handled by processChannels()
            if (samplesProcessed % 500000 == 0) // Log every 500K samples
            {
                juce::Logger::writeToLog("  Channel " + juce::String(channelIndex + 1) + ": " + 
                                        juce::String(channelProgress * 100.0, 1) + "% complete");
            }
        }
    }
    
    // Ensure all data is written
    writerPtr->flush();
    
    return true;
}

//==============================================================================
void AudioFileProcessor::updateProgress(double progress, const juce::String& message)
{
    currentProgress = progress;
    
    // Call progress callback on message thread for UI updates
    if (onProgress)
    {
        auto weak = std::weak_ptr<std::atomic<bool>>(aliveFlag);
        juce::MessageManager::callAsync([this, weak, progress, message]()
        {
            if (auto alive = weak.lock(); alive && alive->load() && onProgress)
                onProgress(progress, message);
        });
    }
}

//==============================================================================
void AudioFileProcessor::completeProcessing()
{
    // Call completion callback on message thread for UI updates
    if (onCompletion)
    {
        // Copy result in thread-safe manner
        ProcessingResult resultCopy;
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            resultCopy = result;
        }

        auto weak = std::weak_ptr<std::atomic<bool>>(aliveFlag);
        juce::MessageManager::callAsync([this, weak, resultCopy]()
        {
            if (auto alive = weak.lock(); alive && alive->load() && onCompletion)
                onCompletion(resultCopy);
        });
    }
}

//==============================================================================
// Thread-safe helper methods

void AudioFileProcessor::setError(const juce::String& errorMessage)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.success = false;
    result.errorMessage = errorMessage;
}

void AudioFileProcessor::addOutputFile(const juce::String& filepath)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.outputFiles.add(filepath);
}

void AudioFileProcessor::incrementSamplesProcessed(juce::int64 samples)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.totalSamplesProcessed += samples;
}

void AudioFileProcessor::setSuccess(bool success)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.success = success;
}

void AudioFileProcessor::setProcessingTime(double seconds)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.processingTimeSeconds = seconds;
}