#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

// Two overlapping Hann-windowed read heads. Phase is independent of ring wrap.
class MengicPitchShifter
{
public:
    void prepare(double sampleRate)
    {
        length = (int)(sampleRate * 0.25) + 8;
        for (auto& channel : ring) channel.assign((size_t)length, 0.0f);
        base = sampleRate * 0.075;
        sweep = sampleRate * 0.055;
        write = 0;
        phase = 0.0;
    }

    void process(float* frame, int channels, double ratio)
    {
        const double second = phase < 0.5 ? phase + 0.5 : phase - 0.5;
        const double weight = 0.5 - 0.5 * std::cos(6.283185307179586 * phase);
        for (int ch = 0; ch < channels; ++ch)
        {
            ring[(size_t)ch][(size_t)write] = frame[ch];
            frame[ch] = (float)(read(ch, phase) * weight + read(ch, second) * (1.0 - weight));
        }
        // Read speed = 1 - delay slope = ratio.
        phase += (1.0 - ratio) / sweep;
        phase -= std::floor(phase);
        if (++write == length) write = 0;
    }

private:
    float read(int channel, double headPhase) const
    {
        double position = (double)write - base - headPhase * sweep;
        if (position < 0.0) position += (double)length;
        const int index = (int)position;
        const float fraction = (float)(position - (double)index);
        const auto& data = ring[(size_t)channel];
        return data[(size_t)index] * (1.0f - fraction)
             + data[(size_t)((index + 1) % length)] * fraction;
    }
    std::array<std::vector<float>, 2> ring;
    int length = 0, write = 0;
    double base = 0.0, sweep = 1.0, phase = 0.0;
};
