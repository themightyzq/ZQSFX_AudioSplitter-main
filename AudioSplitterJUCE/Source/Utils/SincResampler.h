#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

//==============================================================================
/**
 * SincResampler - streaming, mono, arbitrary-ratio sample-rate converter with a Kaiser-windowed
 * sinc low-pass, used for the "Override Sample Rate" option.
 *
 * Why not juce::ResamplingAudioSource: measured, it passes a 40 kHz tone at 96k -> 44.1k through
 * to 4.1 kHz at only -30 dB, a 26 kHz tone to 18.1 kHz at -8 dB, loses 3 dB at 20 kHz, and leaves
 * the mirror image of a 20 kHz tone at -14 dB after 48k -> 96k (Tests/SplitSafetyTest). This
 * converter puts the stopband edge at the lower of the two Nyquist frequencies:
 *   - passband flat to about 0.455 of the lower rate (20.1 kHz when the lower rate is 44.1 kHz),
 *   - stopband from the lower Nyquist, at least 85 dB down (Kaiser beta 8.96, 64 zero crossings),
 * so nothing the new rate cannot represent folds back into the audible band, and upsampling
 * leaves no images. No delay: output sample k is the signal at input time k * (inRate/outRate).
 *
 * Usage: push input, pull output; pull() returns 0 when it needs more input. To flush the tail
 * the caller keeps pushing zeros (the converter treats the signal as silent outside the file).
 * Not thread safe; one instance per stream.
 */
class SincResampler
{
public:
    SincResampler(double inputRate, double outputRate);

    /** Append input samples. */
    void push(const float* samples, int numSamples);

    /** Produce up to `maxOutput` samples from the input pushed so far; returns how many.
        Returns 0 when more input is needed before the next sample can be computed. */
    int pull(float* output, int maxOutput);

private:
    static constexpr int kPhases = 1024;       // fractional-position table resolution
    static constexpr int kZeroCrossings = 64;  // sinc zero crossings each side of centre, in lower-rate samples
    static constexpr double kKaiserBeta = 8.96; // about 90 dB stopband

    const double ratio;        // input samples consumed per output sample
    int halfTaps {0};          // L: taps reach L-1 before and L after the centre sample
    int numTaps {0};           // 2 * L
    std::vector<float> table;  // (kPhases + 1) rows of numTaps coefficients, each row summing to 1

    std::vector<float> buffer; // input history + pending input
    int64_t bufferStart {0};   // absolute input index of buffer[0]
    int64_t nextOutput {0};    // index of the next output sample

    const float* row(int phase) const { return table.data() + (std::size_t) phase * (std::size_t) numTaps; }
    static void dualDot(const float* a, const float* b, const float* x, int n, float& sumA, float& sumB);
};
