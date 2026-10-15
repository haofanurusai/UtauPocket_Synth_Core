#ifndef LLSM_DATABASE_H
#define LLSM_DATABASE_H
#include <stdint.h>
#include "llsm_file_utils.h"
#pragma pack(push, 2)
struct LLSMDiskRecord
{
    uint16_t midi_symbol, ovl, pre, con, vowel_lfrm, vowel_rfrm, nfrm, flag;
};
#pragma pack(pop)
static_assert(sizeof(LLSMDiskRecord) == 16, "database disk layout");
struct LLSMRecord : LLSMDiskRecord
{
    llsm_chunk *chunk = nullptr;
    FP_TYPE inv_overlap = 1;
    LLSMRecord() : LLSMDiskRecord{} {}
    ~LLSMRecord() { llsm_delete_chunk(chunk); }
    LLSMRecord(const LLSMRecord &) = delete;
    LLSMRecord &operator=(const LLSMRecord &) = delete;
};
class LLSMDatabase
{
    static constexpr uint32_t TABLE_SIZE = 128 * 256;
    LLSMRecord records[TABLE_SIZE];

public:
    llsm_soptions *opt_s = nullptr;
    llsm_container *conf = nullptr; // borrowed from the first record
    ~LLSMDatabase() { llsm_delete_soptions(opt_s); }
    int load(const TCHAR *path);
    LLSMRecord *get(uint8_t bank, uint8_t prog)
    {
        if (bank > 127 || prog > 127)
            return nullptr;
        auto &record = records[(uint16_t(prog) << 8) | bank];
        return record.chunk ? &record : nullptr;
    }
};
/* Private frame metadata, not part of the disk/API contract. */
enum
{
    UTAU_FRAME_RD_ORIGINAL = 4,
    UTAU_FRAME_F0_ORIGINAL = 6,
    UTAU_FRAME_PHASE_SHIFTED = 7
};
#endif
