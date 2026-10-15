#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_TCHAR_WRAPPER_IMPLEMENTATION
#include "optparse_tchar_wrapper.h"
#include "cmd_parser.h"
#include <climits>
#include <cstdio>
#include <limits>

void print_usage(const char *program, HostPlatform platform)
{
    std::fprintf(stderr, "Usage: %s <db_file> [options]\n", program);
    std::fprintf(stderr,
                 "  -p, --period-size N    Samples per output block (default 128)\n"
                 "  -n, --period-count N   PCM periods / Windows latency hint (default 2)\n"
                 "  -c, --channels N       Output channels, duplicated mono (default 2)\n"
                 "  -r, --rate N           Output rate; defaults to bank rate, must match it\n"
                 "  -h, --help             Show this help\n");
    if (platform == HostPlatform::Linux)
        std::fprintf(stderr,
                     "  -D, --card N           TinyALSA card (default 0)\n"
                     "  -d, --device N         TinyALSA PCM device (default 0)\n"
                     "  -s, --serial PATH      MIDI serial port (default /dev/ttyS3)\n"
                     "  -b, --baud N           Serial baud rate (default 115200; supports 31250)\n");
    else
        std::fprintf(stderr,
                     "  -l, --list-midi        List MIDI inputs; no database required\n"
                     "  -m, --midi-in N        MIDI input index; automatic only if exactly one\n"
                     "Audio: first ASIO output if present, otherwise the default output.\n");
}
static bool number(const char *s, int minimum, int &value)
{
    if (!s || !*s)
        return false;
    unsigned n = 0;
    for (; *s; ++s)
    {
        if (*s < '0' || *s > '9')
            return false;
        unsigned digit = unsigned(*s - '0');
        if (n > (unsigned(INT_MAX) - digit) / 10)
            return false;
        n = n * 10 + digit;
    }
    if (n < unsigned(minimum))
        return false;
    value = int(n);
    return true;
}
int parse_cmd_args(int argc, TCHAR **argv, PlayerOptions &out, HostPlatform platform)
{
    out = PlayerOptions{};
    optparse_tchar_t options;
    optparse_tchar_init(&options, argc, argv);
    struct Cleanup
    {
        optparse_tchar_t *p;
        ~Cleanup() { optparse_tchar_free(p); }
    } cleanup{&options};
    const struct optparse_long definitions[] = {
        {"card", 'D', OPTPARSE_REQUIRED}, {"device", 'd', OPTPARSE_REQUIRED}, {"serial", 's', OPTPARSE_REQUIRED}, {"baud", 'b', OPTPARSE_REQUIRED}, {"period-size", 'p', OPTPARSE_REQUIRED}, {"period-count", 'n', OPTPARSE_REQUIRED}, {"channels", 'c', OPTPARSE_REQUIRED}, {"rate", 'r', OPTPARSE_REQUIRED}, {"list-midi", 'l', OPTPARSE_NONE}, {"midi-in", 'm', OPTPARSE_REQUIRED}, {"help", 'h', OPTPARSE_NONE}, {nullptr, 0, OPTPARSE_NONE}};
    int option;
    while ((option = optparse_tchar_long(&options, definitions, nullptr)) != -1)
    {
        if (option == 'h')
        {
            print_usage(options.opt.argv[0], platform);
            return 1;
        }
        if (option == '?')
        {
            std::fprintf(stderr, "%s\n", options.errmsg);
            return -1;
        }
        bool serial_option = option == 's' || option == 'b' || option == 'D' || option == 'd';
        bool midi_option = option == 'l' || option == 'm';
        if ((platform == HostPlatform::Windows && serial_option) ||
            (platform == HostPlatform::Linux && midi_option))
        {
            std::fprintf(stderr, "This option is not supported on this platform.\n");
            return -1;
        }
        if (option == 's')
        {
            out.serial = options.optarg_tchar;
            if (!out.serial || !*out.serial)
            {
                std::fprintf(stderr, "Empty serial port.\n");
                return -1;
            }
            continue;
        }
        if (option == 'l')
        {
            out.list_midi = true;
            continue;
        }
        int *destination = nullptr, minimum = 1;
        switch (option)
        {
        case 'D':
            destination = &out.card;
            minimum = 0;
            break;
        case 'd':
            destination = &out.device;
            minimum = 0;
            break;
        case 'b':
            destination = &out.baud;
            break;
        case 'p':
            destination = &out.period_size;
            break;
        case 'n':
            destination = &out.period_count;
            minimum = 2;
            break;
        case 'c':
            destination = &out.channels;
            break;
        case 'r':
            destination = &out.rate;
            break;
        case 'm':
            destination = &out.midi_in;
            minimum = 0;
            break;
        }
        if (!destination || !number(options.opt.optarg, minimum, *destination))
        {
            std::fprintf(stderr, "Invalid numeric argument: %s\n", options.opt.optarg ? options.opt.optarg : "");
            return -1;
        }
    }
    out.database = optparse_tchar_arg(&options);
    if (optparse_tchar_arg(&options))
    {
        std::fprintf(stderr, "Only one database path is accepted.\n");
        return -1;
    }
    if (!out.list_midi && (!out.database || !*out.database))
    {
        print_usage(options.opt.argv[0], platform);
        return -1;
    }
    if (out.period_size > INT_MAX / out.period_count ||
        out.period_size > INT_MAX / out.channels / 2 ||
        size_t(out.period_size) > std::numeric_limits<size_t>::max() / sizeof(double))
    {
        std::fprintf(stderr, "Audio buffer dimensions are too large.\n");
        return -1;
    }
    return 0;
}
