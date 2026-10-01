#include "SincResampler.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    // Modified Bessel function of the first kind, order 0 (series; converges fast for beta <= ~12).
    double besselI0(double x)
    {
        double sum = 1.0, term = 1.0;
        const double q = x * x / 4.0;
        for (int k = 1; k < 60; ++k)
        {
            term *= q / ((double) k * (double) k);
            sum += term;
            if (term < sum * 1.0e-17)
                break;
        }
        return sum;
    }

    double sinc(double x)
    {
        if (std::abs(x) < 1.0e-12)
            return 1.0;
        return std::sin(kPi * x) / (kPi * x);
    }
}

//==============================================================================
SincResampler::SincResampler(double inputRate, double outputRate)
    : ratio(inputRate / outputRate)
{
    // Everything is sized in units of the LOWER of the two rates, so the stopband edge sits at
    // that rate's Nyquist whether converting up or down. Kaiser transition width in cycles per
    // lower-rate sample: (A - 8) / (2.285 * 2 pi * N) with N = 2 * zero crossings.
    const double lowerOverInput = std::min(1.0, outputRate / inputRate);   // lower rate / input rate
    const double transition = 2.855 / (double) kZeroCrossings;
    const double cutoffInInputUnits = lowerOverInput * (0.5 - transition / 2.0); // cycles per input sample

    const double halfWidth = (double) kZeroCrossings / (2.0 * cutoffInInputUnits);  // in input samples
    halfTaps = (int) std::ceil(halfWidth);
    numTaps = 2 * halfTaps;

    const double i0Beta = besselI0(kKaiserBeta);

    table.assign((std::size_t) (kPhases + 1) * (std::size_t) numTaps, 0.0f);
    for (int phase = 0; phase <= kPhases; ++phase)
    {
        const double frac = (double) phase / (double) kPhases;
        std::vector<double> coeffs((std::size_t) numTaps);
        double sum = 0.0;

        for (int j = 0; j < numTaps; ++j)
        {
            const double x = (double) (j - (halfTaps - 1)) - frac;   // tap position relative to the output instant
            const double r = x / halfWidth;
            double w = 0.0;
            if (std::abs(r) < 1.0)
                w = besselI0(kKaiserBeta * std::sqrt(1.0 - r * r)) / i0Beta;
            coeffs[(std::size_t) j] = 2.0 * cutoffInInputUnits * sinc(2.0 * cutoffInInputUnits * x) * w;
            sum += coeffs[(std::size_t) j];
        }

        // Unity gain at DC for every fractional position, so a steady level never ripples.
        for (int j = 0; j < numTaps; ++j)
            table[(std::size_t) phase * (std::size_t) numTaps + (std::size_t) j] = (float) (coeffs[(std::size_t) j] / sum);
    }

    // Before the first input sample the signal is silent: pre-fill the history the first
    // outputs reach back into.
    bufferStart = -(int64_t) halfTaps;
    buffer.assign((std::size_t) halfTaps, 0.0f);
}

//==============================================================================
void SincResampler::push(const float* samples, int numSamples)
{
    buffer.insert(buffer.end(), samples, samples + numSamples);
}

void SincResampler::dualDot(const float* a, const float* b, const float* x, int n, float& sumA, float& sumB)
{
    // Eight independent accumulators per row so the compiler can vectorise the reduction
    // without -ffast-math.
    float a0[8] = {}, b0[8] = {};
    int j = 0;
    for (; j + 8 <= n; j += 8)
        for (int k = 0; k < 8; ++k)
        {
            a0[k] += a[j + k] * x[j + k];
            b0[k] += b[j + k] * x[j + k];
        }
    float sa = 0.0f, sb = 0.0f;
    for (int k = 0; k < 8; ++k) { sa += a0[k]; sb += b0[k]; }
    for (; j < n; ++j) { sa += a[j] * x[j]; sb += b[j] * x[j]; }
    sumA = sa;
    sumB = sb;
}

int SincResampler::pull(float* output, int maxOutput)
{
    int produced = 0;
    const int64_t bufferEnd = bufferStart + (int64_t) buffer.size(); // one past the last absolute index held

    while (produced < maxOutput)
    {
        const double centre = (double) nextOutput * ratio;
        const int64_t i0 = (int64_t) std::floor(centre);

        // Taps read absolute indices i0 - (L - 1) .. i0 + L.
        if (i0 + halfTaps >= bufferEnd)
            break; // need more input

        const double position = (centre - (double) i0) * (double) kPhases;
        const int phase = std::min((int) position, kPhases - 1);
        const float blend = (float) (position - (double) phase);

        const float* x = buffer.data() + (i0 - (halfTaps - 1) - bufferStart);
        float s0 = 0.0f, s1 = 0.0f;
        dualDot(row(phase), row(phase + 1), x, numTaps, s0, s1);

        output[produced++] = s0 + blend * (s1 - s0);
        ++nextOutput;
    }

    // Drop history no later output can reach, in large steps so the erase is amortised.
    const int64_t firstNeeded = (int64_t) std::floor((double) nextOutput * ratio) - (halfTaps - 1);
    const int64_t droppable = firstNeeded - bufferStart;
    if (droppable > 32768)
    {
        buffer.erase(buffer.begin(), buffer.begin() + (std::ptrdiff_t) droppable);
        bufferStart += droppable;
    }

    return produced;
}
