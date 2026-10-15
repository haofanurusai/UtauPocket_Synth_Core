#include "lookup_tables.h"

FP_TYPE SynthTables::freqTable[MIDI_NOTE_COUNT];
FP_TYPE SynthTables::rawPitchBendTable[PITCH_BEND_RANGE];
FP_TYPE *SynthTables::pitchBendTable = SynthTables::rawPitchBendTable + PITCH_BEND_CENTER;
FP_TYPE SynthTables::sineTable[SINE_TABLE_SIZE];
FP_TYPE SynthTables::cc_multiplier_lut_fp[MAX_MIDI_VAL + 1];
uint32_t SynthTables::cc_multiplier_lut_q16[MAX_MIDI_VAL + 1];
bool SynthTables::tablesInitialized = false;

void SynthTables::initTables()
{
    if (tablesInitialized)
        return;

    for (uint32_t i = 0; i < MIDI_NOTE_COUNT; ++i)
    {
        freqTable[i] = A4_FREQ_HZ * std::pow(FP_TYPE(2.0), (int(i) - A4_MIDI_NOTE) / SEMITONES_PER_OCTAVE);
    }

    for (uint32_t i = 0; i < PITCH_BEND_RANGE; ++i)
    {
        FP_TYPE bendNorm = (int(i) - PITCH_BEND_CENTER) / (FP_TYPE)PITCH_BEND_CENTER;
        rawPitchBendTable[i] = std::pow(FP_TYPE(2.0), (bendNorm * MAX_BEND_SEMITONES) / SEMITONES_PER_OCTAVE);
    }

    for (uint32_t i = 0; i < SINE_TABLE_SIZE; ++i)
    {
        sineTable[i] = std::sin(PI_2 * i / SINE_TABLE_SIZE);
    }

    for (int i = 0; i <= MAX_MIDI_VAL; ++i)
    {
        cc_multiplier_lut_fp[i] = FP_TYPE(0.2) * std::pow(FP_TYPE(5), FP_TYPE(i) / 64);
        cc_multiplier_lut_q16[i] = uint32_t(cc_multiplier_lut_fp[i] * Q16_ONE);
    }

    tablesInitialized = true;
}

void HopTables::initInvLength(int max_hop)
{
    inv_length.resize(max_hop + 1);
    for (int i = 1; i <= max_hop; ++i)
        inv_length[i] = FP_TYPE(1) / FP_TYPE(i);
}
