#ifndef UTAU_PLAYER_SAMPLE_CONVERSION_H
#define UTAU_PLAYER_SAMPLE_CONVERSION_H
#include "utau_pocket_config.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
inline void convert_samples(const FP_TYPE *mono, int16_t *output, int frames, int channels)
{
    for (int i = 0; i < frames; ++i)
    {
        if (!std::isfinite(mono[i]))
            throw std::runtime_error("Non-finite synthesized sample");
        // Preserve the old 32767 scale; clamp before conversion to avoid overflow.
        FP_TYPE x = std::max(FP_TYPE(-1), std::min(FP_TYPE(1), mono[i]));
        int16_t sample = static_cast<int16_t>(x * FP_TYPE(32767));
        for (int c = 0; c < channels; ++c)
            *output++ = sample;
    }
}
#endif
