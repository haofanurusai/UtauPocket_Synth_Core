#ifndef SYNTH_COMMON_H
#define SYNTH_COMMON_H

#include "utau_pocket_config.h"
#include <cstdint>
#include <cmath>

using byte = uint8_t;

constexpr uint32_t SINE_TABLE_SIZE = 1024;
constexpr uint32_t MIDI_NOTE_COUNT = 128;
constexpr uint32_t PITCH_BEND_RANGE = 16384;
constexpr int32_t PITCH_BEND_CENTER = 8192;
constexpr uint8_t MAX_MIDI_VAL = 127;
constexpr FP_TYPE MAX_MIDI_VAL_F = FP_TYPE(127.0);

constexpr int A4_MIDI_NOTE = 69;
constexpr FP_TYPE A4_FREQ_HZ = FP_TYPE(440.0);
constexpr FP_TYPE SEMITONES_PER_OCTAVE = FP_TYPE(12.0);
constexpr FP_TYPE MAX_BEND_SEMITONES = FP_TYPE(2.0);

constexpr FP_TYPE BEND_STEPS_PER_SEMITONE = (PITCH_BEND_RANGE / FP_TYPE(2.0)) / MAX_BEND_SEMITONES;

constexpr FP_TYPE MAX_FM_DEPTH_RATIO = FP_TYPE(1.0) / FP_TYPE(6.0);
constexpr FP_TYPE MAX_AM_DEPTH_RATIO = FP_TYPE(1.0);

constexpr FP_TYPE PI_2 = FP_TYPE(6.283185307179586);

constexpr int ADSR_TICK_MUL = 8;
constexpr FP_TYPE LFO_PERIOD_MUL = FP_TYPE(0.25);

constexpr int8_t NOTE_INVALID = -1;

constexpr uint32_t Q16_SHIFT = 16;
constexpr uint32_t Q16_ONE = 1 << 16;

constexpr uint16_t FLAG_OVL_IS_MINUS = 0x0001;

#endif // SYNTH_COMMON_H
