#ifndef ADSR_H
#define ADSR_H

#include "synth_common.h"

class ADSR
{
public:
    enum State
    {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };

    ADSR();

    void setA(byte a);
    void setP(byte p);
    void setD(byte d);
    void setS(byte s);
    void setR(byte r);

    void noteOn(FP_TYPE vel);
    void noteOff();
    void forceIdle();
    FP_TYPE update();

    State getState() const;

private:
    void calcAttackStep();
    void calcDecayStep();
    void calcReleaseStep();

    State state;
    FP_TYPE val;
    uint32_t a_ticks, d_ticks, r_ticks;
    FP_TYPE a_step, d_step, r_step;
    FP_TYPE p_level, s_level, velocity;
    FP_TYPE peak_val, sustain_val;
};

#endif // ADSR_H
