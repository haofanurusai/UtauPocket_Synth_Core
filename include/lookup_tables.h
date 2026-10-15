#ifndef LOOKUP_TABLES_H
#define LOOKUP_TABLES_H

#include "synth_common.h"
#include <vector>

// Keep shared synthesis tables and instance-owned tables in one module.
struct SynthTables
{
    static FP_TYPE freqTable[MIDI_NOTE_COUNT];
    static FP_TYPE rawPitchBendTable[PITCH_BEND_RANGE];
    static FP_TYPE *pitchBendTable;
    static FP_TYPE sineTable[SINE_TABLE_SIZE];
    // MIDI CC -> dimensionless multiplier: 0.2 * 5^(value / 64).
    // 0 -> 0.2, 64 -> 1.0; 127 is approximately 4.876.
    static FP_TYPE cc_multiplier_lut_fp[MAX_MIDI_VAL + 1];
    static uint32_t cc_multiplier_lut_q16[MAX_MIDI_VAL + 1];
    static bool tablesInitialized;
    static void initTables();
};

struct HopTables
{
    std::vector<FP_TYPE> inv_length;
    void initInvLength(int max_hop);
};

#endif // LOOKUP_TABLES_H
