#ifndef SINE_LFO_H
#define SINE_LFO_H

#include "synth_common.h"

class SineLFO
{
public:
    SineLFO();

    void setTicks(byte t);
    FP_TYPE update(const FP_TYPE *sTable); // 需要外部传入正弦表

private:
    FP_TYPE phase;
    FP_TYPE phase_inc;
};

#endif // SINE_LFO_H