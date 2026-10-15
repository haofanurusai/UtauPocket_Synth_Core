#ifndef SYNTH_CTRL_H
#define SYNTH_CTRL_H

#include "synth_common.h"
#include "lookup_tables.h"
#include "adsr.h"
#include "sine_lfo.h"
#include "note_tracker.h"

class SynthCtrl : private SynthTables
{
public:
    SynthCtrl();

    void update();
    FP_TYPE getF0() const;
    FP_TYPE getVol() const;
    bool active() const;

    void handleNoteOn(byte channel, byte note, byte velocity);
    void handleNoteOff(byte channel, byte note, byte velocity);
    void handlePitchBend(byte channel, int bend);
    void handleControlChange(byte channel, byte number, byte value);

    void allSoundOff();
    void allNotesOff();

private:
    FP_TYPE currentF0;
    FP_TYPE currentVol;

    NoteTracker noteTracker;
    FP_TYPE startPitch;
    FP_TYPE targetPitch;
    FP_TYPE currentPitch;
    uint32_t glideTicks;
    uint32_t glideProgress;
    FP_TYPE glideInvTicks;

    int currentBendIndex;
    FP_TYPE fmDepth;
    FP_TYPE amDepth;

    ADSR mainADSR;
    ADSR fmADSR;
    ADSR amADSR;
    SineLFO fmLFO;
    SineLFO amLFO;

};

#endif // SYNTH_CTRL_H
