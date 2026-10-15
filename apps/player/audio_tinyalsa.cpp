#include "platform.h"
#include <tinyalsa/asoundlib.h>
#include <stdexcept>
#include <string>
#include <cerrno>
struct AudioOutput::Impl
{
    pcm *handle = nullptr;
    unsigned channels = 0;
    ~Impl()
    {
        if (handle)
            pcm_close(handle);
    }
};
AudioOutput::AudioOutput(const PlayerOptions &options, int rate) : impl(new Impl)
{
    pcm_config config{};
    config.channels = options.channels;
    config.rate = rate;
    config.period_size = options.period_size;
    config.period_count = options.period_count;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = options.period_size;
    config.stop_threshold = options.period_size * options.period_count;
    // pcm_writei uses interleaved write transfers, not the mmap API.
    impl->handle = pcm_open(options.card, options.device, PCM_OUT | PCM_MMAP, &config);
    if (!impl->handle || !pcm_is_ready(impl->handle))
        throw std::runtime_error(std::string("Cannot open PCM output: ") + (impl->handle ? pcm_get_error(impl->handle) : "out of memory"));
    if (pcm_get_rate(impl->handle) != unsigned(rate) || pcm_get_channels(impl->handle) != unsigned(options.channels))
        throw std::runtime_error("PCM device changed the requested rate or channel count");
    impl->channels = options.channels;
}
AudioOutput::~AudioOutput() = default;
void AudioOutput::write(const int16_t *samples, int frames)
{
    while (frames > 0)
    {
        int written = pcm_writei(impl->handle, samples, unsigned(frames));
        if (written == -EINTR)
            throw std::runtime_error("PCM write interrupted");
        if (written <= 0 || written > frames)
            throw std::runtime_error(std::string("PCM write failed: ") + pcm_get_error(impl->handle));
        samples += size_t(written) * impl->channels;
        frames -= written;
    }
}
