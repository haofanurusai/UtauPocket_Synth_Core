#ifndef VOCAL_STATE_MACHINE_H
#define VOCAL_STATE_MACHINE_H
#include "synth_common.h"
#include "lookup_tables.h"
#include "llsm_database.h"
#include "llsmrt.h"

class VocalStateMachine : private SynthTables
{
public:
    enum State
    {
        IDLE,
        CONSONANT,
        VOWEL_START,
        VOWEL_LOOP_FWD,
        VOWEL_LOOP_BWD
    };
    struct SynthVoice
    {
        LLSMRecord *record = nullptr;
        uint32_t frame_q16 = 0;
        State state = IDLE;
        llsm_rtsynth_buffer *synth = nullptr;
        FP_TYPE gain = 0, target = 0, ratio = 1;
        ~SynthVoice() { llsm_delete_rtsynth_buffer(synth); }
        void reset()
        {
            if (synth)
                llsm_rtsynth_buffer_clear(synth);
            record = nullptr;
            frame_q16 = 0;
            state = IDLE;
            gain = target = 0;
            ratio = 1;
        }
        int frame() const { return state == IDLE ? -1 : int(frame_q16 >> Q16_SHIFT); }
    };
    explicit VocalStateMachine(LLSMDatabase *db);
    void handleControlChange(byte channel, byte cc, byte value);
    void handleProgramChange(byte channel, byte program);
    void allSoundOff();
    bool update();
    SynthVoice voices[2];
    SynthVoice *current = &voices[0], *fading = &voices[1];

private:
    LLSMDatabase *db;
    byte bank_char = 0, prog_char = 0, cc_con_speed = 64, cc_vow_speed = 64;
    bool crossfading = false;
    FP_TYPE fading_start = 0;
    void advance(SynthVoice &v);
};
#endif
