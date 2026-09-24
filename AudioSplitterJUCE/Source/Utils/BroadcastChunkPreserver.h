#pragma once

#include <juce_core/juce_core.h>

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

    /** Convenience overload using defaultChunkIds(). */
    bool preserve (const juce::File& source, const juce::File& dest, juce::String& message);
}
