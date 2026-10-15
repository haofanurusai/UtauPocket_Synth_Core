#include "platform.h"
#include "portaudio.h"
#include <cstdio>
#include <stdexcept>
#include <string>
namespace
{
    void check(PaError status, const char *operation)
    {
        if (status != paNoError)
            throw std::runtime_error(std::string(operation) + ": " + Pa_GetErrorText(status));
    }
}
struct AudioOutput::Impl
{
    PaStream *stream = nullptr;
    bool initialized = false, started = false, warned = false;
    ~Impl()
    {
        if (stream)
        {
            if (started)
                Pa_StopStream(stream);
            Pa_CloseStream(stream);
        }
        if (initialized)
            Pa_Terminate();
    }
};
AudioOutput::AudioOutput(const PlayerOptions &options, int rate) : impl(new Impl)
{
    check(Pa_Initialize(), "PortAudio initialization failed");
    impl->initialized = true;
    int count = Pa_GetDeviceCount();
    if (count < 0)
        check(count, "Audio device enumeration failed");
    PaDeviceIndex device = paNoDevice;
    for (int i = 0; i < count; ++i)
    {
        auto info = Pa_GetDeviceInfo(i);
        auto api = info ? Pa_GetHostApiInfo(info->hostApi) : nullptr;
        if (info && api && info->maxOutputChannels > 0 && api->type == paASIO)
        {
            device = i;
            break;
        }
    }
    if (device == paNoDevice)
        device = Pa_GetDefaultOutputDevice();
    auto info = Pa_GetDeviceInfo(device);
    if (!info)
        throw std::runtime_error("No audio output device");
    PaStreamParameters output{};
    output.device = device;
    output.channelCount = options.channels;
    output.sampleFormat = paInt16;
    output.suggestedLatency = double(options.period_size) * options.period_count / rate;
    check(Pa_IsFormatSupported(nullptr, &output, rate), "Audio device does not support bank rate/channels");
    check(Pa_OpenStream(&impl->stream, nullptr, &output, rate, options.period_size, paClipOff, nullptr, nullptr), "Cannot open audio output");
    check(Pa_StartStream(impl->stream), "Cannot start audio output");
    impl->started = true;
    std::fprintf(stderr, "Audio output: %s\n", info->name);
}
AudioOutput::~AudioOutput() = default;
void AudioOutput::write(const int16_t *samples, int frames)
{
    PaError status = Pa_WriteStream(impl->stream, samples, static_cast<unsigned long>(frames));
    if (status == paOutputUnderflowed || status == paTimedOut)
    {
        if (!impl->warned)
        {
            std::fprintf(stderr, "Warning: %s\n", Pa_GetErrorText(status));
            impl->warned = true;
        }
        return; // The supplied block has still been written.
    }
    check(status, "Audio write failed");
}
