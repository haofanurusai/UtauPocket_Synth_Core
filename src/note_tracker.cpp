#include "note_tracker.h"

NoteTracker::NoteTracker()
{
    memset(prev, NOTE_INVALID, sizeof(prev));
    memset(next, NOTE_INVALID, sizeof(next));
    topNote = NOTE_INVALID;
}

void NoteTracker::noteOn(int8_t note)
{
    noteOff(note);

    prev[note] = topNote;
    next[note] = NOTE_INVALID;

    if (topNote != NOTE_INVALID)
    {
        next[topNote] = note;
    }
    topNote = note;
}

void NoteTracker::noteOff(int8_t note)
{
    int8_t p = prev[note];
    int8_t n = next[note];

    if (p == NOTE_INVALID && n == NOTE_INVALID && topNote != note)
        return;

    if (p != NOTE_INVALID)
        next[p] = n;
    if (n != NOTE_INVALID)
        prev[n] = p;

    if (topNote == note)
    {
        topNote = p;
    }

    prev[note] = NOTE_INVALID;
    next[note] = NOTE_INVALID;
}

int8_t NoteTracker::getTopNote() const
{
    return topNote;
}