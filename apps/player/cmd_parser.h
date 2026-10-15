#ifndef UTAU_PLAYER_CMD_PARSER_H
#define UTAU_PLAYER_CMD_PARSER_H
#include "utau_pocket_tchar.h"
enum class HostPlatform
{
    Linux,
    Windows
};
struct PlayerOptions
{
    const TCHAR *database = nullptr;
    const TCHAR *serial = _T("/dev/ttyS3");
    int baud = 115200, card = 0, device = 0;
    int period_size = 128, period_count = 2, channels = 2, rate = 0;
    int midi_in = -1;
    bool list_midi = false;
};
// 0: parsed; 1: help displayed; -1: invalid arguments. Strings borrow argv.
int parse_cmd_args(int argc, TCHAR **argv, PlayerOptions &out, HostPlatform platform);
void print_usage(const char *program, HostPlatform platform);
#endif
