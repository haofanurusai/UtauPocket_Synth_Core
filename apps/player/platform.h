#ifndef UTAU_PLAYER_PLATFORM_H
#define UTAU_PLAYER_PLATFORM_H
#include "cmd_parser.h"
#include <cstddef>
#include <cstdint>
#include <memory>
class MidiInput
{
public:
    explicit MidiInput(const PlayerOptions &options);
    ~MidiInput();
    static void list();
    size_t read(uint8_t *buffer, size_t capacity);
#ifdef __linux__
    void sendTuneReply();
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
class AudioOutput
{
public:
    AudioOutput(const PlayerOptions &options, int sample_rate);
    ~AudioOutput();
    void write(const int16_t *samples, int frames);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
#endif
