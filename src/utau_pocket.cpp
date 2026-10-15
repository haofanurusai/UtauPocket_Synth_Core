#include "utau_pocket.h"
#include "synth_ctrl.h"
#include "vocal_state_machine.h"
#include "lookup_tables.h"
#include "midi_input.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <vector>

namespace
{
    struct CoreState : HopTables
    {
        LLSMDatabase database;
        SynthCtrl control;
        VocalStateMachine vocal{&database};
        MemoryTransport transport;
        CoreMidi midi{transport};
        utau_pocket_tune_request_callback tune_request_callback = nullptr;
        void *tune_request_user_data = nullptr;
        std::vector<FP_TYPE> hop_buffer;
        int fs = 0, block = 0, hop = 0, max_hop = 0, remaining = 0, position = 0;
        FP_TYPE phase = 0, omega = 0, volume = 0, ap_gain = 1;
        FP_TYPE main_volume = 1, aperiodic_gain = 1, rd_multiplier = 1;
        bool fault = false;
        int init(const TCHAR *path, int length);
        int render();
    };
    CoreState *core = nullptr;
    void note_on(byte c, byte n, byte v) { core->control.handleNoteOn(c, n, v); }
    void note_off(byte c, byte n, byte v) { core->control.handleNoteOff(c, n, v); }
    void set_conf_multiplier(int member, byte value)
    {
        FP_TYPE scaled = *static_cast<FP_TYPE *>(llsm_container_get(core->database.conf, member)) *
                         SynthTables::cc_multiplier_lut_fp[value];
        // Update both owned CONF copies, including the currently idle voice.
        for (auto &v : core->vocal.voices)
            *static_cast<FP_TYPE *>(llsm_container_get(llsm_rtsynth_buffer_getconf(v.synth), member)) = scaled;
    }
    void control_change(byte c, byte cc, byte v)
    {
        switch (cc)
        {
        case 7: core->main_volume = FP_TYPE(v) / MAX_MIDI_VAL_F; return;
        case 118: set_conf_multiplier(LLSM_CONF_FNYQ, v); return;
        case 119: core->aperiodic_gain = SynthTables::cc_multiplier_lut_fp[v]; return;
        case 20: core->rd_multiplier = SynthTables::cc_multiplier_lut_fp[v]; return;
        case 21: set_conf_multiplier(LLSM_CONF_LIPRADIUS, v); return;
        }
        core->vocal.handleControlChange(c, cc, v);
        core->control.handleControlChange(c, cc, v);
    }
    void program_change(byte c, byte p) { core->vocal.handleProgramChange(c, p); }
    void pitch_bend(byte c, int b) { core->control.handlePitchBend(c, b); }
    void tune_request()
    {
        if (core->tune_request_callback)
            core->tune_request_callback(core->tune_request_user_data);
    }

    int CoreState::init(const TCHAR *path, int length)
    {
        int status = database.load(path);
        if (status)
            return status;
        fs = int(database.opt_s->fs);
        block = length;
        FP_TYPE thop = *static_cast<FP_TYPE *>(llsm_container_get(database.conf, LLSM_CONF_THOP));
        omega = PI_2 * thop;
        hop = int(std::lround(double(fs)*double(thop)));
        max_hop = int(std::ceil(double(thop) * fs)) + 4;
        hop_buffer.resize(max_hop);
        initInvLength(max_hop);
        for (auto &v : vocal.voices)
        {
            v.synth = llsm_create_rtsynth_buffer(database.opt_s, database.conf, max_hop * 2);
            if (!v.synth)
                return LLSM_ERR_MEM_ALLOC;
        }
        midi.begin(MIDI_CHANNEL_OMNI);
        midi.turnThruOff();
        midi.setHandleNoteOn(note_on);
        midi.setHandleNoteOff(note_off);
        midi.setHandleControlChange(control_change);
        midi.setHandleProgramChange(program_change);
        midi.setHandlePitchBend(pitch_bend);
        midi.setHandleTuneRequest(tune_request);
        return fs;
    }
    void tune(llsm_container *frame, FP_TYPE f0, FP_TYPE phase, FP_TYPE rd_multiplier, FP_TYPE &ratio)
    {
        auto original = static_cast<FP_TYPE *>(llsm_container_get(frame, UTAU_FRAME_F0_ORIGINAL));
        auto shifted = static_cast<FP_TYPE *>(llsm_container_get(frame, UTAU_FRAME_PHASE_SHIFTED));
        *static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_F0)) = f0;
        ratio = *original < 1 ? FP_TYPE(1) : (*original / f0);
        auto rd = static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_RD));
        auto original_rd = static_cast<FP_TYPE *>(llsm_container_get(frame, UTAU_FRAME_RD_ORIGINAL));
        if (rd && original_rd)
            *rd = *original_rd * rd_multiplier;
        llsm_container_remove(frame, LLSM_FRAME_HM);
        llsm_frame_phaseshift(frame, phase - *shifted);
        *shifted = phase;
    }
    int CoreState::render()
    {
        control.update();
        bool playing = control.active() && vocal.update();
        if (!playing)
        {
            // Pause both LLSM instances and retain phase and interpolation history.
            std::fill_n(hop_buffer.data(), hop, FP_TYPE(0));
            position = 0;
            remaining = hop;
            return 0;
        }
        FP_TYPE f0 = control.getF0(), next_volume = control.getVol() * main_volume;
        if (!std::isfinite(f0) || f0 <= 0 || !std::isfinite(next_volume))
            return LLSM_ERR_INVALID_DATA;
        int length = 0;
        for (auto &v : vocal.voices)
        {
            if (llsm_rtsynth_buffer_numoutput(v.synth) != 0)
                return LLSM_ERR_INVALID_STATE;
            v.ratio = FP_TYPE(1);
            int index = v.frame();
            if (index < 0)
                continue;
            if (index >= v.record->nfrm)
                return LLSM_ERR_INVALID_DATA;
            llsm_container *frame = v.record->chunk->frames[index];
            tune(frame, f0, phase, rd_multiplier, v.ratio);
            llsm_rtsynth_buffer_feed(v.synth, frame);
            int count = llsm_rtsynth_buffer_numoutput(v.synth);
            if (count <= 0 || count > max_hop)
                return LLSM_ERR_INVALID_STATE;
            length = std::max(length, count);
        }
        if (length <= 0)
            return LLSM_ERR_INVALID_STATE;
        phase = std::fmod(omega * f0 + phase, PI_2);
        auto &a = *vocal.current, &b = *vocal.fading;
        FP_TYPE vol_step = (next_volume - volume) * inv_length[length];
        FP_TYPE step_a = (a.target - a.gain) * inv_length[length], step_b = (b.target - b.gain) * inv_length[length];
        FP_TYPE ap_step = (aperiodic_gain - ap_gain) * inv_length[length];
        for (int i = 0; i < length; ++i)
        {
            // ap/bp are periodic; an/bn are aperiodic and scaled by CC119.
            FP_TYPE ap = 0, an = 0, bp = 0, bn = 0;
            // Empty fetch returns 0 without writing: exhausted/idle lanes stay zero.
            llsm_rtsynth_buffer_fetch_decomposed(a.synth, &ap, &an);
            llsm_rtsynth_buffer_fetch_decomposed(b.synth, &bp, &bn);
            volume += vol_step;
            a.gain += step_a;
            b.gain += step_b;
            ap_gain += ap_step;
            hop_buffer[i] = ((ap * a.ratio + an * ap_gain) * a.gain +
                             (bp * b.ratio + bn * ap_gain) * b.gain) * volume;
            if (!std::isfinite(hop_buffer[i]))
                return LLSM_ERR_INVALID_DATA;
        }
        volume = next_volume;
        a.gain = a.target;
        b.gain = b.target;
        ap_gain = aperiodic_gain;
        position = 0;
        remaining = length;
        return 0;
    }
}
extern "C"
{
    int utau_pocket_init(const TCHAR *path, int block)
    {
        if (core)
            return LLSM_ERR_INVALID_STATE;
        if (!path)
            return LLSM_ERR_NULL_PTR;
        if (!*path || block <= 0 || size_t(block) > std::numeric_limits<size_t>::max() / sizeof(FP_TYPE))
            return LLSM_ERR_INVALID_ARGUMENT;
        try
        {
            std::unique_ptr<CoreState> candidate(new CoreState);
            int result = candidate->init(path, block);
            if (result > 0)
                core = candidate.release();
            return result;
        }
        catch (const std::bad_alloc &)
        {
            return LLSM_ERR_MEM_ALLOC;
        }
        catch (...)
        {
            return LLSM_ERR_INVALID_DATA;
        }
    }
    void utau_pocket_close(void)
    {
        delete core;
        core = nullptr;
    }
    void utau_pocket_set_tune_request_callback(
        utau_pocket_tune_request_callback callback, void *user_data)
    {
        if (core)
        {
            core->tune_request_callback = callback;
            core->tune_request_user_data = callback ? user_data : nullptr;
        }
    }
    int utau_pocket_push_midi_evt(const uint8_t *evt, size_t len)
    {
        if (!core || core->fault)
            return LLSM_ERR_INVALID_STATE;
        if (len && !evt)
            return LLSM_ERR_NULL_PTR;
        core->transport.bind(evt, len);
        try
        {
            while (core->transport.available())
            {
                size_t before = core->transport.cursor;
                core->midi.read(); // false can mean an incomplete message.
                if (before == core->transport.cursor)
                {
                    core->transport.bind(nullptr, 0);
                    core->fault = true;
                    return LLSM_ERR_INVALID_STATE;
                }
            }
            core->transport.bind(nullptr, 0);
            return LLSM_ERR_OK;
        }
        catch (...)
        {
            core->transport.bind(nullptr, 0);
            core->fault = true;
            return LLSM_ERR_INVALID_STATE;
        }
    }
    int utau_pocket_synth(FP_TYPE *buf)
    {
        if (!buf)
            return LLSM_ERR_NULL_PTR;
        if (!core)
            return LLSM_ERR_INVALID_STATE;
        int status = core->fault ? LLSM_ERR_INVALID_STATE : 0;
        try
        {
            int written = 0;
            while (!status && written < core->block)
            {
                if (!core->remaining)
                {
                    status = core->render();
                    if (status)
                        break;
                }
                int n = std::min(core->remaining, core->block - written);
                std::copy_n(core->hop_buffer.data() + core->position, n, buf + written);
                written += n;
                core->position += n;
                core->remaining -= n;
            }
        }
        catch (const std::bad_alloc &)
        {
            status = LLSM_ERR_MEM_ALLOC;
        }
        catch (...)
        {
            status = LLSM_ERR_INVALID_STATE;
        }
        if (status)
        {
            std::fill_n(buf, core->block, FP_TYPE(0));
            core->fault = true;
            return status;
        }
        return core->block;
    }
}
