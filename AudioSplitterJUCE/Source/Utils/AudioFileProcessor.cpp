#include "AudioFileProcessor.h"
#include "BroadcastChunkPreserver.h"

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
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result.success = false;
            result.errorMessage = "Input file path is empty";
        }
        // Outside the lock: completeProcessing() takes resultMutex itself.
        completeProcessing();
        return;
    }

    if (options.outputDirectory.isEmpty())
    {
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result.success = false;
            result.errorMessage = "Output directory is empty";
        }
        // Outside the lock: completeProcessing() takes resultMutex itself.
        completeProcessing();
        return;
    }

    if (options.selectedChannels.empty())
    {
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result.success = false;
            result.errorMessage = "No channels selected for extraction";
        }
        // Outside the lock: completeProcessing() takes resultMutex itself.
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
        // Single-pass pipeline: every failure path breaks out of this block (the error is
        // already recorded by setError) and so reaches the one completeProcessing() call
        // at the end of run(). Never `return` from inside it.
        do
        {
            // Step 1: Load and validate input file (10% of progress)
            updateProgress(0.0, "Loading input file...");
            if (!loadInputFile())
                break; // Error already set

            if (threadShouldExit() || shouldCancel.load())
            {
                setError("Processing cancelled");
                break;
            }

            updateProgress(0.1, "Input file loaded successfully");

            // Step 2: Setup output directory (20% of progress)
            updateProgress(0.1, "Setting up output directory...");
            if (!setupOutputDirectory())
                break;

            updateProgress(0.2, "Output directory ready");

            // Step 3: Process channels (20% - 90% of progress)
            updateProgress(0.2, "Processing audio channels...");
            if (!processChannels())
                break;

            // Step 4: Complete successfully (100%)
            updateProgress(1.0, "Processing completed successfully");

            auto endTime = juce::Time::getMillisecondCounterHiRes();
            setProcessingTime((endTime - startTime) / 1000.0);
            setSuccess(true);

            juce::Logger::writeToLog("AudioFileProcessor: Processing completed in " +
                                    juce::String((endTime - startTime) / 1000.0, 2) + " seconds");
        } while (false);
    }
    catch (const std::exception& e)
    {
        setError("Processing error: " + juce::String(e.what()));
        juce::Logger::writeToLog("AudioFileProcessor: Exception caught - " + juce::String(e.what()));
    }
    catch (...)
    {
        setError("Unknown processing error occurred");
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
        setError("Input file does not exist: " + currentOptions.inputFilePath);
        return false;
    }

    // Create audio format reader - mirrors Python audio file loading (lines 693-702)
    auto* reader = formatManager.createReaderFor(inputFile);
    if (reader == nullptr)
    {
        setError("Cannot read audio file (unsupported format): " + currentOptions.inputFilePath);
        return false;
    }

    audioReader.reset(reader);

    // Validate channel selection against actual file
    int actualChannels = (int)audioReader->numChannels;
    for (int channelIndex : currentOptions.selectedChannels)
    {
        if (channelIndex < 0 || channelIndex >= actualChannels)
        {
            setError("Invalid channel index " + juce::String(channelIndex + 1) +
                     " (file has " + juce::String(actualChannels) + " channels)");
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
            setError("Cannot create output directory: " + createResult.getErrorMessage());
            return false;
        }
    }

    if (!outputDir.isDirectory())
    {
        setError("Output path is not a directory: " + currentOptions.outputDirectory);
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
            if ((int)i < currentOptions.ucsChannelSuffixes.size())
            {
                channelSuffix = currentOptions.ucsChannelSuffixes[(int)i];
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
        if ((int)i < customNames.size() && customNames[(int)i].isNotEmpty())
        {
            channelName = customNames[(int)i];
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
    
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.outputFiles.clear();
    }

    // Process each selected channel - mirrors Python channel extraction loop (lines 734-820)
    for (size_t i = 0; i < currentOptions.selectedChannels.size(); ++i)
    {
        if (threadShouldExit() || shouldCancel.load())
        {
            setError("Processing cancelled");
            return false;
        }

        int channelIndex = currentOptions.selectedChannels[i];
        juce::String outputPath = outputFilenames[(int) i];
        juce::String channelName = juce::File(outputPath).getFileNameWithoutExtension().fromLastOccurrenceOf("_", false, false);
        
        double channelProgressStart = 0.2 + (i * progressPerChannel);
        updateProgress(channelProgressStart, "Processing " + channelName + "...");
        
        if (!extractChannel(channelIndex, outputPath))
        {
            return false; // Error already set
        }

        addOutputFile(outputPath);
        
        double channelProgressEnd = 0.2 + ((i + 1) * progressPerChannel);
        updateProgress(channelProgressEnd, "Completed " + channelName);
        
        juce::Logger::writeToLog("AudioFileProcessor: Extracted channel " + juce::String(channelIndex + 1) + 
                                " to " + juce::File(outputPath).getFileName());
    }
    
    return true;
}

//==============================================================================
bool AudioFileProcessor::extractChannel(int channelIndex, const juce::String& outputPath)
{
    juce::File outputFile(outputPath);
    
    // Delete existing file if it exists
    if (outputFile.exists())
        outputFile.deleteFile();
    
    // Create audio format writer - mirrors Python audio writer creation
    auto outputStream = outputFile.createOutputStream();
    if (!outputStream)
    {
        setError("Cannot create output file: " + outputPath);
        return false;
    }

    // Get appropriate audio format (WAV by default)
    auto* format = formatManager.findFormatForFileExtension(outputFile.getFileExtension());
    if (!format)
    {
        setError("Unsupported output format: " + outputFile.getFileExtension());
        return false;
    }
    
    // Determine output sample rate and bit depth
    double outputSampleRate = (currentOptions.sampleRate > 0) ? currentOptions.sampleRate : audioReader->sampleRate;
    int outputBitDepth = (currentOptions.bitDepth > 0) ? currentOptions.bitDepth : (int)audioReader->bitsPerSample;

    // Prepare metadata for output file - preserves BWF BEXT, iXML, and all other metadata
    juce::StringPairArray outputMetadata;

    if (currentOptions.preserveMetadata && audioReader->metadataValues.size() > 0)
    {
        // Hand JUCE the source metadata so it round-trips the BWF `bext` fields Soundminer
        // reads (Description, Originator, OriginatorReference, dates/times, TimeReference,
        // CodingHistory) plus any cue/smpl/INFO data. NOTE: JUCE does NOT faithfully
        // reconstruct classic production iXML (SCENE/TAKE/TAPE/PROJECT/TRACK_LIST) -- that
        // chunk is copied verbatim at the RIFF level after the file is written, by
        // BroadcastChunkPreserver. Do not add per-field "mono" rewrites here: JUCE's keys
        // (e.g. "bwav coding history") are not the ones earlier code assumed, so such edits
        // were silent no-ops.
        outputMetadata = audioReader->metadataValues;

        // Log key BWF fields once (first channel) using JUCE's actual metadata keys.
        if (channelIndex == currentOptions.selectedChannels[0])
        {
            const juce::String description = outputMetadata.getValue(juce::WavAudioFormat::bwavDescription, "(none)");
            const juce::String originator  = outputMetadata.getValue(juce::WavAudioFormat::bwavOriginator, "(none)");
            const juce::String timeRef     = outputMetadata.getValue(juce::WavAudioFormat::bwavTimeReference, "(none)");

            juce::Logger::writeToLog("  Preserving metadata: " + juce::String(outputMetadata.size()) + " field(s)");
            juce::Logger::writeToLog("    BWF Description: " + description);
            juce::Logger::writeToLog("    BWF Originator: " + originator);
            juce::Logger::writeToLog("    BWF TimeReference: " + timeRef);
        }
    }
    else
    {
        if (audioReader->metadataValues.size() > 0)
        {
            juce::Logger::writeToLog("  Metadata preservation disabled - metadata will not be copied");
        }
    }

    // Create writer with single channel output and preserved metadata.
    // createWriterFor takes ownership of the stream on success.
    // On failure, we must not leak the stream.
    auto* rawStream = outputStream.get();
    auto* writer = format->createWriterFor(rawStream,
                                           outputSampleRate,
                                           1, // Single channel output
                                           outputBitDepth,
                                           outputMetadata,  // Pass metadata to preserve BWF/iXML
                                           0);

    if (!writer)
    {
        // Stream was NOT consumed -- unique_ptr will clean it up
        setError("Cannot create audio writer for: " + outputPath);
        return false;
    }

    // Writer took ownership of the stream, release unique_ptr to avoid double-free
    outputStream.release();
    std::unique_ptr<juce::AudioFormatWriter> writerPtr(writer);
    
    // Check if we need to resample
    bool needsResampling = (! juce::approximatelyEqual(outputSampleRate, audioReader->sampleRate))
                           && (currentOptions.sampleRate > 0);
    
    // Process audio in chunks to avoid memory issues and provide progress updates
    const int bufferSize = 8192; // Process in 8K sample chunks
    juce::AudioBuffer<float> readBuffer((int)audioReader->numChannels, bufferSize);
    juce::AudioBuffer<float> writeBuffer(1, bufferSize); // Single channel output
    
    juce::int64 totalSamples = audioReader->lengthInSamples;
    juce::int64 samplesProcessed = 0;
    
    // Set up resampling if needed
    std::unique_ptr<juce::ResamplingAudioSource> resampler;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
    // For resampling, create a fresh reader so position starts at 0
    // (the shared audioReader may be at EOF from a previous channel extraction)
    std::unique_ptr<juce::AudioFormatReader> channelReader;

    if (needsResampling)
    {
        // Create independent reader for this channel to avoid shared position state
        juce::File inputFile(currentOptions.inputFilePath);
        channelReader.reset(formatManager.createReaderFor(inputFile));
        if (!channelReader)
        {
            setError("Cannot re-open input file for resampling: " + currentOptions.inputFilePath);
            return false;
        }

        // Create audio source from fresh reader
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(channelReader.get(), false);

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
            setError("Processing cancelled");
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
    
    // Ensure all data is written, then destroy the writer so the output file handle
    // is closed before we re-open the file to preserve broadcast metadata chunks.
    writerPtr->flush();
    writerPtr.reset();

    incrementSamplesProcessed(samplesProcessed);

    // Preserve broadcast metadata chunks (iXML/axml) that JUCE's WAV writer drops.
    // This is the root-cause fix for classic field-recorder iXML loss (SCENE/TAKE/
    // TAPE/PROJECT/TRACK_LIST); see BroadcastChunkPreserver and Tests/MetadataRoundTripTest.
    // Failure here is non-fatal: the audio is already correct, so we warn rather than
    // discard the channel (CLAUDE.md H.2 graceful degradation).
    if (currentOptions.preserveMetadata)
    {
        juce::File inputFile(currentOptions.inputFilePath);
        juce::String preserveMessage;
        const bool preserved = BroadcastChunkPreserver::preserve(inputFile, outputFile, preserveMessage);
        juce::Logger::writeToLog(juce::String("  [metadata] ")
                                 + (preserved ? "" : "WARNING: ") + preserveMessage);
    }

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

//==============================================================================
AudioFileProcessor::ProcessingResult AudioFileProcessor::waitForResult(int timeoutMs)
{
    waitForThreadToExit(timeoutMs);
    std::lock_guard<std::mutex> lock(resultMutex);
    return result;
}

AudioFileProcessor::ProcessingResult AudioFileProcessor::getResult() const
{
    std::lock_guard<std::mutex> lock(resultMutex);
    return result;
}