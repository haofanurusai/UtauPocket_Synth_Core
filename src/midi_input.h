#ifndef UTAU_MIDI_INPUT_H
#define UTAU_MIDI_INPUT_H
#include "MIDI.h"
#include <cstddef>
#include <climits>
struct CoreMidiSettings : midi::DefaultSettings {
    static const bool Use1ByteParsing = true;
    static const bool UseSenderActiveSensing = false;
    static const bool UseReceiverActiveSensing = false;
};
struct MemoryTransport {
    static const bool thruActivated = false;
    const uint8_t *data=nullptr;
    size_t size=0, cursor=0;
    void begin() {}
    unsigned available() const { size_t n=size-cursor;return n>UINT_MAX ? UINT_MAX : unsigned(n); }
    uint8_t read() { return data[cursor++]; }
    bool beginTransmission(midi::MidiType) { return false; }
    void write(uint8_t) {}
    void endTransmission() {}
    void bind(const uint8_t *p,size_t n) { data=p;size=n;cursor=0; }
};
using CoreMidi = midi::MidiInterface<MemoryTransport,CoreMidiSettings>;
#endif
