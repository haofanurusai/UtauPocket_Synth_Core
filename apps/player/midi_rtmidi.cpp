#include "platform.h"
#include "RtMidi.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>
namespace
{
    void list_ports(RtMidiIn &input)
    {
        unsigned n = input.getPortCount();
        for (unsigned i = 0; i < n; ++i)
            std::printf("%u: %s\n", i, input.getPortName(i).c_str());
        if (!n)
            std::puts("No MIDI input ports found.");
    }
}
struct MidiInput::Impl
{
    RtMidiIn input{RtMidi::WINDOWS_MM, "UtauPocket", 1024};
    std::vector<unsigned char> message;
    Impl() { message.reserve(256); }
};
MidiInput::MidiInput(const PlayerOptions &options) : impl(new Impl)
{
    unsigned count = impl->input.getPortCount();
    int port = options.midi_in;
    if (port < 0)
    {
        if (count != 1)
        {
            list_ports(impl->input);
            throw std::runtime_error("Select a MIDI input using --midi-in N");
        }
        port = 0;
    }
    if (unsigned(port) >= count)
        throw std::runtime_error("MIDI input index is out of range; use --list-midi");
    impl->input.ignoreTypes(true, true, true); // Unused SysEx, timing and sensing.
    std::fprintf(stderr, "MIDI input %d: %s\n", port, impl->input.getPortName(unsigned(port)).c_str());
    impl->input.openPort(unsigned(port), "UtauPocket input");
}
MidiInput::~MidiInput() = default;
void MidiInput::list()
{
    RtMidiIn input(RtMidi::WINDOWS_MM, "UtauPocket");
    list_ports(input);
}
size_t MidiInput::read(uint8_t *buffer, size_t capacity)
{
    // The return value is a delta timestamp, NOT success; zero is valid.
    impl->input.getMessage(&impl->message);
    if (impl->message.size() > capacity)
        throw std::runtime_error("MIDI message exceeds input buffer");
    std::copy(impl->message.begin(), impl->message.end(), buffer);
    return impl->message.size();
}
