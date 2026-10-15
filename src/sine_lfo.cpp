#include "sine_lfo.h"

SineLFO::SineLFO() : phase(FP_TYPE(0.0)), phase_inc(FP_TYPE(0.0)) {}

void SineLFO::setTicks(byte t)
{
    phase_inc = (t > 0) ? (LFO_PERIOD_MUL / t) : FP_TYPE(0.0);
}

FP_TYPE SineLFO::update(const FP_TYPE *sTable)
{
    if (phase_inc == FP_TYPE(0.0))
        return FP_TYPE(0.0);

    phase += phase_inc;
    if (phase >= FP_TYPE(1.0))
        phase -= FP_TYPE(1.0);

    uint32_t index = (uint32_t)(phase * SINE_TABLE_SIZE) & (SINE_TABLE_SIZE - 1);
    return sTable[index];
}
