#include "utau_pocket.h"
#include "cmd_parser.h"
#include "platform.h"
#include "sample_conversion.h"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif
namespace
{
    volatile std::sig_atomic_t stopped = 0;
    void stop(int) { stopped = 1; }
    struct CoreLifetime
    {
        ~CoreLifetime() { utau_pocket_close(); }
    };
#ifdef __linux__
    struct TuneRequestRegistration
    {
        explicit TuneRequestRegistration(MidiInput &midi)
        {
            utau_pocket_set_tune_request_callback([](void *user_data) {
                static_cast<MidiInput *>(user_data)->sendTuneReply();
            }, &midi);
        }
        ~TuneRequestRegistration()
        {
            utau_pocket_set_tune_request_callback(nullptr, nullptr);
        }
    };
#endif
}
#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
{
    constexpr HostPlatform platform = HostPlatform::Windows;
    SetConsoleOutputCP(CP_UTF8);
#else
int main(int argc, char **argv)
{
    constexpr HostPlatform platform = HostPlatform::Linux;
#endif
    try
    {
        PlayerOptions options;
        int parsed = parse_cmd_args(argc, argv, options, platform);
        if (parsed)
            return parsed > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        if (options.list_midi)
        {
            MidiInput::list();
            return EXIT_SUCCESS;
        }
        CoreLifetime lifetime;
        std::printf("Reading voicebank...\n");
        std::fflush(stdout);
        int rate = utau_pocket_init(options.database, options.period_size);
        if (rate <= 0)
        {
            std::fprintf(stderr, "Cannot load voicebank: LLSM error %d\n", rate);
            return EXIT_FAILURE;
        }
        std::printf("Voicebank loaded successfully.\n");
        std::fflush(stdout);
        if (options.rate && options.rate != rate)
        {
            std::fprintf(stderr, "Requested rate %d differs from bank rate %d; no resampler is enabled.\n", options.rate, rate);
            return EXIT_FAILURE;
        }
        std::vector<FP_TYPE> mono(options.period_size);
        std::vector<int16_t> output(size_t(options.period_size) * options.channels);
        MidiInput midi(options);
#ifdef __linux__
        TuneRequestRegistration tune_request_registration(midi);
#endif
        AudioOutput audio(options, rate);
        std::signal(SIGINT, stop);
        std::signal(SIGTERM, stop);
#ifdef SIGBREAK
        std::signal(SIGBREAK, stop);
#endif
        std::fprintf(stderr, "Playing: %d Hz, %d samples/block, %d channel(s). Ctrl+C to stop.\n",
                     rate, options.period_size, options.channels);
        uint8_t bytes[256];
        while (!stopped)
        {
            // Bound input work so a continuous stream cannot starve audio output.
            for (int batch = 0; batch < 16 && !stopped; ++batch)
            {
                size_t n = midi.read(bytes, sizeof(bytes));
                if (!n)
                    break;
                int status = utau_pocket_push_midi_evt(bytes, n);
                if (status)
                    throw std::runtime_error("MIDI parser error " + std::to_string(status));
            }
            if (stopped)
                break;
            int status = utau_pocket_synth(mono.data());
            if (status != options.period_size)
                throw std::runtime_error("Synthesis error " + std::to_string(status));
            convert_samples(mono.data(), output.data(), options.period_size, options.channels);
            audio.write(output.data(), options.period_size);
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception &error)
    {
        if (stopped)
            return EXIT_SUCCESS;
        std::fprintf(stderr, "utau_pocket: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
