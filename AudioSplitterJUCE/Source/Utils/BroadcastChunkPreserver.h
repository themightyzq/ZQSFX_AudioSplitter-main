#pragma once

#include <juce_core/juce_core.h>
#include <functional>

/**
    BroadcastChunkPreserver
    -----------------------
    Preserves broadcast metadata RIFF chunks that juce::WavAudioFormat silently
    drops on a read -> write round-trip.

    WHY THIS EXISTS (root cause, not a workaround):
    juce::WavAudioFormat reconstructs the `iXML` chunk from a curated set of
    ASWG-namespaced metadata keys only (see juce_WavAudioFormat.cpp `IXMLChunk`).
    Classic production iXML written by field recorders (Sound Devices, Zoom) -- the
    <SCENE>, <TAKE>, <TAPE>, <PROJECT>, <TRACK_LIST> elements that Soundminer and UCS
    workflows depend on -- is NOT among those keys, so JUCE writes no usable iXML on
    output. This is proven by Tests/MetadataRoundTripTest. The correct layer to fix it
    is the RIFF container itself: copy the affected chunks byte-for-byte from the source
    file into the freshly written output. That is what this class does.

    The chunks are read from the source by streaming its RIFF directory (never loading
    the audio data), and appended to the output after its existing chunks. Trailing
    metadata chunks are valid RIFF and are read correctly by JUCE and Soundminer; this
    avoids rewriting the (potentially multi-GB) audio data that an insert-before-`data`
    would require.

    Failure to preserve is reported but is NON-FATAL: the audio output is already
    correct, and the caller should warn rather than discard the file (CLAUDE.md H.2:
    graceful degradation -- preserve what you can and warn).
*/
namespace BroadcastChunkPreserver
{
    /** The broadcast chunks JUCE fails to round-trip faithfully. Order is preserved on output.
        Note: `bext` is intentionally NOT here -- JUCE round-trips the bext fields Soundminer
        reads, and re-adding it raw would duplicate the chunk. */
    juce::StringArray defaultChunkIds();

    /** Copies each requested RIFF chunk verbatim from `source` into `dest`.

        A chunk is appended only if it exists in `source` AND is not already present in
        `dest` (avoids duplicating an ASWG-derived iXML JUCE may have written).

        @param source     the original multichannel WAV to copy chunks from
        @param dest       the freshly written output WAV to append chunks to
        @param chunkIds   4-character RIFF chunk IDs (e.g. "iXML", "axml")
        @param message    [out] human-readable detail (what was copied, or why not)
        @returns          true if every applicable chunk was preserved (or none were
                          needed); false if a condition prevented full preservation
                          (RF64 dest, 4GB overflow, malformed RIFF, I/O error). On false,
                          `dest` is left unmodified or with only the chunks that succeeded.
    */
    bool preserve (const juce::File& source,
                   const juce::File& dest,
                   const juce::StringArray& chunkIds,
                   juce::String& message);

    /** Optional edit applied to each chunk's bytes after it is read from the source and before it
        is written to `dest` (used to rewrite sample-rate fields when the audio was converted).
        It may change the size. Leave empty to copy verbatim. */
    using ChunkTransform = std::function<void (const juce::String& chunkId, juce::MemoryBlock& payload)>;

    /** As above, applying `transform` to each chunk that is copied. */
    bool preserve (const juce::File& source,
                   const juce::File& dest,
                   const juce::StringArray& chunkIds,
                   juce::String& message,
                   const ChunkTransform& transform);

    /** Convenience overload using defaultChunkIds(). */
    bool preserve (const juce::File& source, const juce::File& dest, juce::String& message);

    //==========================================================================
    // Sample-rate (and bit-depth) conversion: keeping the metadata true to the OUTPUT file.
    //
    // After a conversion the copied fields would otherwise describe the SOURCE: rates, and counts
    // of samples at the source rate. Fields rewritten:
    //   iXML  FILE_SAMPLE_RATE, TIMESTAMP_SAMPLE_RATE -> output rate
    //         TIMESTAMP_SAMPLES_SINCE_MIDNIGHT_HI/LO  -> rescaled to the output rate
    //         AUDIO_BIT_DEPTH                         -> output bit depth
    //   bext  TimeReference                           -> rescaled to the output rate
    //         CodingHistory                           -> one EBU R98 line recording the output
    //                                                    format and the conversion (the source's
    //                                                    own history is kept)
    //   smpl/cue (as JUCE exposes them): SamplePeriod, loop start/end and cue offsets, rescaled.
    // Left alone on purpose: iXML DIGITIZER_SAMPLE_RATE (the rate the recorder captured at, which
    // is still true), SPEED/timecode frame rates, and opaque cue-region (ltxt) data.

    /** Returns `ixml` with the rate and sample-count fields above rewritten. A tag that is not
        present is not added. `outputBitDepth` 0 leaves AUDIO_BIT_DEPTH alone. */
    juce::String retargetIXml (const juce::String& ixml, double sourceRate, double outputRate, int outputBitDepth);

    /** Rewrites the same fields in JUCE's WAV metadata map (the bext, smpl and cue values that
        WavAudioFormat reads and writes). `outputBitDepth` is used in the coding-history line.
        Returns a short description of what changed, for the log. */
    juce::String retargetMetadataValues (juce::StringPairArray& values, double sourceRate, double outputRate, int outputBitDepth);
}
