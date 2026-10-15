#include "vocal_state_machine.h"
#include <algorithm>
#include <cmath>

VocalStateMachine::VocalStateMachine(LLSMDatabase *bank) : db(bank)
{
    initTables();
}
void VocalStateMachine::handleControlChange(byte, byte cc, byte value)
{
    switch (cc)
    {
    case 32:
        bank_char = value;
        break;
    case 116:
        cc_con_speed = value;
        break;
    case 117:
        cc_vow_speed = value;
        break;
    case 120:
        allSoundOff();
        break;
    }
}
void VocalStateMachine::handleProgramChange(byte, byte program)
{
    if (!bank_char)
        return;
    prog_char = program;
    auto next = db->get(bank_char, prog_char);
    bank_char = 0;
    if (!next)
        return;
    std::swap(current, fading);
    // Reuse the oldest lane after clearing its previous synthesis history.
    current->reset();
    current->record = next;
    current->state = CONSONANT;
    if (fading->state == IDLE)
    {
        fading->reset();
        crossfading = false;
        current->gain = current->target = FP_TYPE(1);
    }
    else
    {
        crossfading = true;
        fading_start = fading->gain;
        current->gain = current->target = (next->flag & FLAG_OVL_IS_MINUS)
                                             ? FP_TYPE(1) : FP_TYPE(1) - fading_start;
    }
}
void VocalStateMachine::allSoundOff()
{
    crossfading = false;
    current->reset();
    fading->reset();
}
void VocalStateMachine::advance(SynthVoice &v)
{
    if (v.state == IDLE)
        return;
    uint64_t step = cc_multiplier_lut_q16[v.state == CONSONANT ? cc_con_speed : cc_vow_speed];
    uint64_t left = uint64_t(v.record->vowel_lfrm) << Q16_SHIFT;
    uint64_t right = uint64_t(v.record->vowel_rfrm) << Q16_SHIFT;
    if (v.state == CONSONANT || v.state == VOWEL_START)
    {
        uint64_t pos = uint64_t(v.frame_q16) + step;
        if (v.state == CONSONANT && (pos >> Q16_SHIFT) >= v.record->con)
            v.state = VOWEL_START;
        if (v.state == VOWEL_START && pos >= left)
            v.state = VOWEL_LOOP_FWD;
        if (pos < left)
        {
            v.frame_q16 = uint32_t(pos);
            return;
        }
        // Fold arbitrary overshoot, including loops shorter than one speed step.
        uint64_t width = right - left, offset = (pos - left) % (2 * width);
        v.frame_q16 = uint32_t(offset <= width ? left + offset : right - (offset - width));
        v.state = offset < width ? VOWEL_LOOP_FWD : VOWEL_LOOP_BWD;
        return;
    }
    uint64_t width = right - left;
    uint64_t offset = v.state == VOWEL_LOOP_FWD ? uint64_t(v.frame_q16) - left : width + right - uint64_t(v.frame_q16);
    offset = (offset + step) % (2 * width);
    v.frame_q16 = uint32_t(offset <= width ? left + offset : right - (offset - width));
    v.state = offset < width ? VOWEL_LOOP_FWD : VOWEL_LOOP_BWD;
}
bool VocalStateMachine::update()
{
    advance(*current);
    advance(*fading);
    if (crossfading)
    {
        FP_TYPE progress = std::min(FP_TYPE(1), FP_TYPE(current->frame_q16 >> Q16_SHIFT) * current->record->inv_overlap);
        fading->target = fading_start * (FP_TYPE(1) - progress);
        current->target = (current->record->flag & FLAG_OVL_IS_MINUS)
                              ? FP_TYPE(1) : FP_TYPE(1) - fading->target;
        // Keep the old voice alive until its final ramp has been mixed.
        if (progress >= 1 && fading->gain == 0)
        {
            crossfading = false;
            fading->state = IDLE;
            fading->record = nullptr;
        }
    }
    return current->state != IDLE || fading->state != IDLE;
}
