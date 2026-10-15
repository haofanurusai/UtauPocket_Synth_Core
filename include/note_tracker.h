#ifndef NOTE_TRACKER_H
#define NOTE_TRACKER_H

#include "synth_common.h"
#include <cstring>

class NoteTracker
{
private:
    int8_t prev[MIDI_NOTE_COUNT];
    int8_t next[MIDI_NOTE_COUNT];
    int8_t topNote;

public:
    NoteTracker();
    void noteOn(int8_t note);
    void noteOff(int8_t note);
    int8_t getTopNote() const;
};

#endif // NOTE_TRACKER_H