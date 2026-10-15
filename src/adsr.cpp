#include "adsr.h"

ADSR::ADSR()
    : state(IDLE), val(FP_TYPE(0.0)), a_ticks(1), d_ticks(1), r_ticks(1),
      a_step(FP_TYPE(0.0)), d_step(FP_TYPE(0.0)), r_step(FP_TYPE(0.0)),
      p_level(FP_TYPE(1.0)), s_level(FP_TYPE(1.0)), velocity(FP_TYPE(0.0)),
      peak_val(FP_TYPE(0.0)), sustain_val(FP_TYPE(0.0)) {}

void ADSR::setA(byte a)
{
    a_ticks = a * ADSR_TICK_MUL;
    // if (state == ATTACK) calcAttackStep();
}

void ADSR::setD(byte d)
{
    d_ticks = d * ADSR_TICK_MUL;
    // if (state == DECAY) calcDecayStep();
}

void ADSR::setP(byte p)
{
    p_level = p / MAX_MIDI_VAL_F;
    // if(state == ATTACK) calcAttackStep();
    // else if(state == DECAY) calcDecayStep();
}

void ADSR::setS(byte s)
{
    s_level = s / MAX_MIDI_VAL_F;
    // if (state == DECAY) calcDecayStep();
}

void ADSR::setR(byte r)
{
    r_ticks = r * ADSR_TICK_MUL;
    // if (state == RELEASE) calcReleaseStep();
}

void ADSR::noteOn(FP_TYPE vel)
{
    velocity = vel;
    peak_val = p_level * velocity;
    sustain_val = s_level * velocity;
    /*if (a_ticks == 0) {
        val = velocity;
        state = (d_ticks == 0) ? SUSTAIN : DECAY;
        calcDecayStep();
    } else */
    {
        // val = FP_TYPE(0.0);
        state = ATTACK;
        calcAttackStep();
    }
}

void ADSR::noteOff()
{
    if (state != IDLE)
    {
        state = RELEASE;
        calcReleaseStep();
    }
}

void ADSR::forceIdle()
{
    state = IDLE;
    val = FP_TYPE(0.0);
}

FP_TYPE ADSR::update()
{
    switch (state)
    {
    case ATTACK:
        val += a_step;
        if (val >= peak_val)
        {
            val = peak_val;
            state = /*(d_ticks == 0) ? SUSTAIN : */ DECAY;
            calcDecayStep();
        }
        break;
    case DECAY:
        val -= d_step;
        if ((d_step >= FP_TYPE(0.0) && val <= sustain_val) || (d_step < FP_TYPE(0.0) && val >= sustain_val))
        {
            val = sustain_val;
            state = SUSTAIN;
        }
        break;
    case SUSTAIN:
        val = sustain_val;
        break;
    case RELEASE:
        val -= r_step;
        if (val <= FP_TYPE(0.0))
        {
            val = FP_TYPE(0.0);
            state = IDLE;
        }
        break;
    case IDLE:
        val = FP_TYPE(0.0);
        break;
    }
    return val;
}

ADSR::State ADSR::getState() const
{
    return state;
}

void ADSR::calcAttackStep()
{
    a_step = /*(a_ticks == 0) ? velocity : */ (peak_val / (FP_TYPE)a_ticks);
}

void ADSR::calcDecayStep()
{
    d_step = /*(d_ticks == 0) ? FP_TYPE(0.0) : */ ((peak_val - sustain_val) / (FP_TYPE)d_ticks);
}

void ADSR::calcReleaseStep()
{
    r_step = /*(r_ticks == 0) ? val : */ (val / (FP_TYPE)r_ticks);
}
