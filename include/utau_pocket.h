#ifndef UTAU_POCKET_H
#define UTAU_POCKET_H
#include <stddef.h>
#include <stdint.h>
#include "utau_pocket_config.h"
#include "utau_pocket_tchar.h"
#include "llsm_errors.h"
#if defined(_WIN32) && defined(UTAU_POCKET_SHARED)
#ifdef UTAU_POCKET_BUILD
#define UTAU_POCKET_API __declspec(dllexport)
#else
#define UTAU_POCKET_API __declspec(dllimport)
#endif
#elif defined(__GNUC__)
#define UTAU_POCKET_API __attribute__((visibility("default")))
#else
#define UTAU_POCKET_API
#endif
#ifdef __cplusplus
extern "C"
{
#endif
    /* Single instance. Serialize all calls; do not call recursively.
     * path: database index, Windows Unicode / POSIX native path bytes.
     * Returns positive sample rate or LLSM_ERR_*. Failure leaves no instance.
     * An already initialized instance is left unchanged (INVALID_STATE). */
    UTAU_POCKET_API int utau_pocket_init(const TCHAR *path, int blk_size);
    /* Idempotent. */
    UTAU_POCKET_API void utau_pocket_close(void);
    /* Optional synchronous Tune Request (F6) notification. Register after init;
     * NULL disables it. Cleared on close. The callback must not call core APIs. */
    typedef void (*utau_pocket_tune_request_callback)(void *user_data);
    UTAU_POCKET_API void utau_pocket_set_tune_request_callback(
        utau_pocket_tune_request_callback callback, void *user_data);
    /* Writes exactly blk_size mono FP_TYPE samples; returns blk_size or error.
     * Output is unclipped normalized floating point. A runtime failure zeros
     * the entire block and requires close/init. Invalid arguments do not advance.
     * Ticks follow the bank hop, independent of blk_size. Active lanes use the
     * maximum available sample count; silent ticks use int(fs * thop), as in the
     * original main. Idle lanes pause rather than consume empty input frames. */
    UTAU_POCKET_API int utau_pocket_synth(FP_TYPE *buf);
    /* Synchronously consumes MIDI 1.0 bytes and dispatches complete messages.
     * Partial messages persist, input pointers do not. No queue or timestamp.
     * Returns 0 on success. NULL,0 is valid after init. Events affect the next
     * not-yet-generated hop; audio already buffered is not rewritten. */
    UTAU_POCKET_API int utau_pocket_push_midi_evt(const uint8_t *evt, size_t len);
#ifdef __cplusplus
}
#endif
#endif
