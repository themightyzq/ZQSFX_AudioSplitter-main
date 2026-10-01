#include "AudioFileProcessor.h"
#include "BroadcastChunkPreserver.h"
#include "SincResampler.h"

#include <algorithm>
#include <cmath>

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
void AudioFileProcessor::requestCancel()
{
    juce::Logger::writeToLog("AudioFileProcessor: Cancellation requested");
    shouldCancel = true;
    signalThreadShouldExit();
}

void AudioFileProcessor::cancelProcessing()
{
    requestCancel();
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

            if (isCancelRequested())
            {
                setCancelled();
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
bool AudioFileProcessor::buildOutputPlans(std::vector<OutputPlan>& plans)
{
    plans.clear();

    const juce::File inputFile(currentOptions.inputFilePath);
    const juce::File outputDir(currentOptions.outputDirectory);
    const juce::String extension = inputFile.getFileExtension();
    const auto& selected = currentOptions.selectedChannels;

    // Stereo-to-mono only means something for a stereo source with both channels chosen.
    const bool mixToMono = currentOptions.stereoToMono
                           && audioReader != nullptr && audioReader->numChannels == 2
                           && std::find(selected.begin(), selected.end(), 0) != selected.end()
                           && std::find(selected.begin(), selected.end(), 1) != selected.end();

    const bool useUCS = currentOptions.useUCSNaming
                        && currentOptions.ucsCategory.isNotEmpty()
                        && currentOptions.ucsDescription.isNotEmpty();

    // Parse custom channel names once - mirrors Python custom naming (lines 716-732)
    juce::StringArray customNames;
    if (currentOptions.customChannelNames.isNotEmpty())
    {
        customNames = juce::StringArray::fromTokens(currentOptions.customChannelNames, ",", "");
        for (int i = 0; i < customNames.size(); ++i)
            customNames.getReference(i) = customNames[i].trim();
    }

    auto ucsName = [&](const juce::String& channelSuffix)
    {
        // Category_Subcategory_Description_Channel.wav
        juce::String filename = currentOptions.ucsCategory;
        if (currentOptions.ucsSubcategory.isNotEmpty())
            filename += "_" + currentOptions.ucsSubcategory;
        filename += "_" + currentOptions.ucsDescription + "_" + channelSuffix + extension;
        return filename;
    };

    if (mixToMono)
    {
        const juce::String filename = useUCS ? ucsName("M")
                                             : inputFile.getFileNameWithoutExtension() + "_mono" + extension;
        plans.push_back({ 0, 1, outputDir.getChildFile(filename) });
    }
    else
    {
        for (size_t i = 0; i < selected.size(); ++i)
        {
            const int channelIndex = selected[i];
            juce::String filename;

            if (useUCS)
            {
                const juce::String suffix = (int) i < currentOptions.ucsChannelSuffixes.size()
                                                ? currentOptions.ucsChannelSuffixes[(int) i]
                                                : "Ch" + juce::String(channelIndex + 1);
                filename = ucsName(suffix);
            }
            else
            {
                // Custom name if provided, otherwise "chanN" (1-based for the user) - Python line 782
                const juce::String channelName = ((int) i < customNames.size() && customNames[(int) i].isNotEmpty())
                                                     ? customNames[(int) i]
                                                     : "chan" + juce::String(channelIndex + 1);
                filename = inputFile.getFileNameWithoutExtension() + "_" + channelName + extension;
            }

            plans.push_back({ channelIndex, -1, outputDir.getChildFile(filename) });
        }
    }

    // Two outputs with one name would silently overwrite each other; an output that is the
    // source would destroy the recording. Refuse both before anything is written. Names are
    // compared case-insensitively because the default macOS and Windows volumes are.
    juce::StringArray seen;
    for (const auto& plan : plans)
    {
        if (plan.target == inputFile)
        {
            setError("Output file would overwrite the source recording: " + plan.target.getFullPathName()
                     + ". Choose a different output folder or channel names.");
            return false;
        }

        const juce::String key = plan.target.getFullPathName().toLowerCase();
        if (seen.contains(key))
        {
            setError("Two channels would be written to the same file name (" + plan.target.getFileName()
                     + "). Give each channel a different name.");
            return false;
        }
        seen.add(key);
    }

    return true;
}

//==============================================================================
bool AudioFileProcessor::processChannels()
{
    std::vector<OutputPlan> plans;
    if (!buildOutputPlans(plans))
        return false;

    {
        std::lock_guard<std::mutex> lock(resultMutex);
        result.outputFiles.clear();
    }

    // Phase 1: write + verify every output into a temporary file next to its target. The
    // TemporaryFile objects delete their files when this function returns without committing
    // (failure or cancel), so an aborted run leaves no partial output and touches no original.
    std::vector<std::unique_ptr<juce::TemporaryFile>> staged;
    const double progressPerOutput = 0.7 / (double) plans.size(); // channel work spans 20% - 90%

    for (size_t i = 0; i < plans.size(); ++i)
    {
        if (isCancelRequested())
        {
            setCancelled();
            return false;
        }

        const juce::String name = plans[i].target.getFileName();
        updateProgress(0.2 + (double) i * progressPerOutput, "Processing " + name + "...");

        staged.push_back(std::make_unique<juce::TemporaryFile>(plans[i].target));
        if (!writeOutputFile(plans[i], staged.back()->getFile()))
            return false; // error or cancel already recorded

        updateProgress(0.2 + (double) (i + 1) * progressPerOutput, "Completed " + name);
    }

    // Phase 2: every temporary is complete and verified; swap them over the targets. Replacing
    // is a rename, so each target is either its old file or its new one, never half-written.
    for (size_t i = 0; i < plans.size(); ++i)
    {
        if (!staged[i]->overwriteTargetFileWithTemporary())
        {
            juce::String message = "Could not replace '" + plans[i].target.getFileName()
                                 + "' in " + plans[i].target.getParentDirectory().getFullPathName()
                                 + ". The existing file was left in place";
            if (i > 0)
                message += "; " + juce::String((int) i) + " earlier file(s) of this run were already replaced";
            setError(message + ". Check that the file is not open in another program and the folder is writable.");
            return false;
        }

        addOutputFile(plans[i].target.getFullPathName());
        juce::Logger::writeToLog("AudioFileProcessor: wrote " + plans[i].target.getFileName());
    }

    return true;
}

//==============================================================================
bool AudioFileProcessor::writeOutputFile(const OutputPlan& plan, const juce::File& destination)
{
    const juce::String targetName = plan.target.getFileName();

    // The destination is a temporary beside the target, so a read-only or full folder fails
    // here, before any existing file has been touched.
    auto outputStream = destination.createOutputStream();
    if (!outputStream)
    {
        setError("Cannot create output file in " + plan.target.getParentDirectory().getFullPathName()
                 + ". Check that the folder exists and is writable.");
        return false;
    }

    // Get appropriate audio format (WAV by default). Looked up from the final name so the
    // temporary's own name never matters.
    auto* format = formatManager.findFormatForFileExtension(plan.target.getFileExtension());
    if (!format)
    {
        setError("Unsupported output format: " + plan.target.getFileExtension());
        return false;
    }

    // Determine output sample rate and bit depth
    const double outputSampleRate = (currentOptions.sampleRate > 0) ? currentOptions.sampleRate : audioReader->sampleRate;
    const int outputBitDepth = (currentOptions.bitDepth > 0) ? currentOptions.bitDepth : (int) audioReader->bitsPerSample;

    if (!format->getPossibleBitDepths().contains(outputBitDepth))
    {
        setError("Cannot write " + juce::String(outputBitDepth) + "-bit audio to " + targetName
                 + ". Choose 16, 24 or 32 bit in Options.");
        return false;
    }

    const bool needsResampling = (! juce::approximatelyEqual(outputSampleRate, audioReader->sampleRate))
                                 && (currentOptions.sampleRate > 0);

    // A converted file must not carry metadata that describes the source's format.
    const bool convertsFormat = needsResampling || outputBitDepth != (int) audioReader->bitsPerSample;

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

        if (convertsFormat)
        {
            const auto changed = BroadcastChunkPreserver::retargetMetadataValues(outputMetadata, audioReader->sampleRate,
                                                                                outputSampleRate, outputBitDepth);
            juce::Logger::writeToLog("  [metadata] output format differs from source; updated: " + changed);
        }
    }
    else if (audioReader->metadataValues.size() > 0)
    {
        juce::Logger::writeToLog("  Metadata preservation disabled - metadata will not be copied");
    }

    // createWriterFor takes ownership of the stream on success; on failure the unique_ptr
    // still owns it and cleans it up.
    auto* writer = format->createWriterFor(outputStream.get(),
                                           outputSampleRate,
                                           1, // Single channel output
                                           outputBitDepth,
                                           outputMetadata,
                                           0);

    if (!writer)
    {
        setError("Cannot create audio writer for " + targetName + " at "
                 + juce::String(outputSampleRate, 0) + " Hz, " + juce::String(outputBitDepth) + " bit.");
        return false;
    }

    outputStream.release(); // the writer owns it now
    std::unique_ptr<juce::AudioFormatWriter> writerPtr(writer);

    // Process audio in chunks to avoid memory issues and provide progress updates
    const int bufferSize = 8192;
    juce::AudioBuffer<float> readBuffer((int) audioReader->numChannels, bufferSize);
    juce::AudioBuffer<float> writeBuffer(1, bufferSize);

    juce::int64 totalSamples = audioReader->lengthInSamples;
    juce::int64 samplesProcessed = 0;

    // Rate conversion: SincResampler (anti-aliased; see its header for why not juce's resampler).
    std::unique_ptr<SincResampler> resampler;
    juce::int64 inputRead = 0;

    if (needsResampling)
    {
        resampler = std::make_unique<SincResampler>(audioReader->sampleRate, outputSampleRate);
        totalSamples = static_cast<juce::int64>(std::llround((double) totalSamples * outputSampleRate / audioReader->sampleRate));
    }

    const juce::int64 expectedSamples = totalSamples;

    // Reads `count` source samples starting at `start` into writeBuffer channel 0: the planned
    // channel, or the equal-gain mix of a stereo pair. Reading past the end of the file gives
    // silence, which is what the resampler's tail needs.
    auto readSource = [&](juce::int64 start, int count) -> bool
    {
        if (!audioReader->read(&readBuffer, 0, count, start, true, true))
            return false;

        writeBuffer.copyFrom(0, 0, readBuffer, plan.sourceChannel, 0, count);
        if (plan.mixPartnerChannel >= 0)
        {
            writeBuffer.addFrom(0, 0, readBuffer, plan.mixPartnerChannel, 0, count);
            writeBuffer.applyGain(0, 0, count, 0.5f);
        }
        return true;
    };

    auto readFailed = [&](juce::int64 at)
    {
        setError("Read error while processing " + targetName + " at sample " + juce::String(at)
                 + ". The source file may be damaged or on a disconnected drive.");
    };

    while (samplesProcessed < totalSamples)
    {
        if (currentOptions.blockHookForTesting)
            currentOptions.blockHookForTesting(samplesProcessed);

        if (isCancelRequested())
        {
            setCancelled();
            return false;
        }

        int samplesToWrite = juce::jmin(bufferSize, (int) (totalSamples - samplesProcessed));

        if (resampler != nullptr)
        {
            // Feed input until the converter can produce output, then take what it has.
            int produced = 0;
            while ((produced = resampler->pull(writeBuffer.getWritePointer(0), samplesToWrite)) == 0)
            {
                if (isCancelRequested())
                {
                    setCancelled();
                    return false;
                }

                if (!readSource(inputRead, bufferSize))
                {
                    readFailed(inputRead);
                    return false;
                }
                resampler->push(writeBuffer.getReadPointer(0), bufferSize);
                inputRead += bufferSize;
            }
            samplesToWrite = produced;
        }
        else if (!readSource(samplesProcessed, samplesToWrite))
        {
            readFailed(samplesProcessed);
            return false;
        }

        if (!writerPtr->writeFromAudioSampleBuffer(writeBuffer, 0, samplesToWrite))
        {
            setError("Write failed for " + targetName + ". The disk may be full or the drive disconnected.");
            return false;
        }

        samplesProcessed += samplesToWrite;
    }

    // Flush, then destroy the writer so the file handle is closed (and the WAV header
    // finalised) before the preserver re-opens the file.
    if (!writerPtr->flush())
    {
        setError("Could not finish writing " + targetName + ". The disk may be full.");
        return false;
    }
    writerPtr.reset();

    // Preserve broadcast metadata chunks (iXML/axml) that JUCE's WAV writer drops. This is the
    // root-cause fix for classic field-recorder iXML loss; see BroadcastChunkPreserver and
    // Tests/MetadataRoundTripTest. Failure here is non-fatal: the audio is already correct, so
    // we warn rather than discard the channel (CLAUDE.md H.2 graceful degradation).
    if (currentOptions.preserveMetadata)
    {
        juce::String preserveMessage;
        BroadcastChunkPreserver::ChunkTransform transform;
        if (convertsFormat)
        {
            // The iXML chunk is copied byte for byte unless the output's format differs from the
            // source's; then its rate and sample-count fields are rewritten to describe the output.
            const double sourceRate = audioReader->sampleRate;
            transform = [sourceRate, outputSampleRate, outputBitDepth](const juce::String& chunkId, juce::MemoryBlock& payload)
            {
                if (chunkId != "iXML")
                    return;

                const auto original = juce::String::fromUTF8(static_cast<const char*>(payload.getData()), (int) payload.getSize());
                const auto updated = BroadcastChunkPreserver::retargetIXml(original, sourceRate, outputSampleRate, outputBitDepth);
                if (updated != original)
                    payload = juce::MemoryBlock(updated.toRawUTF8(), updated.getNumBytesAsUTF8());
            };
        }

        const bool preserved = BroadcastChunkPreserver::preserve(juce::File(currentOptions.inputFilePath), destination,
                                                                 BroadcastChunkPreserver::defaultChunkIds(),
                                                                 preserveMessage, transform);
        juce::Logger::writeToLog(juce::String("  [metadata] ") + (preserved ? "" : "WARNING: ") + preserveMessage);
    }

    // Verify before it can replace anything: the finished file must open as a mono WAV of the
    // expected rate and length.
    std::unique_ptr<juce::AudioFormatReader> check(formatManager.createReaderFor(destination));
    if (check == nullptr || check->numChannels != 1 || check->lengthInSamples != expectedSamples
        || std::abs(check->sampleRate - outputSampleRate) > 0.5)
    {
        setError("The written file " + targetName + " failed verification (unreadable, truncated or wrong format). "
                 "The existing file, if any, was not replaced.");
        return false;
    }

    incrementSamplesProcessed(samplesProcessed);
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

void AudioFileProcessor::setCancelled()
{
    std::lock_guard<std::mutex> lock(resultMutex);
    result.success = false;
    result.wasCancelled = true;
    result.errorMessage = "Processing cancelled";
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