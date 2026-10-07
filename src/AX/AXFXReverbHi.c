/*
 * AX/AXFXReverbHi.c - the AXFX reverb API wrappers (init, shutdown, settings, callback) for the three-channel
 *    body in `AX/AXFXReverbHiExp.c`.
 *
 * RANGE. .text 0x80474CB0-0x80474DD0 (4 functions, 0x120 B); .sdata2 0x8079CFF0-0x8079CFF8 (0.0f, 1.0f).  Right
 *    edge: the start of `AX/AXFXReverbStd.c`, proven by the pool: the four-channel wrappers reload 0.0f and 1.0f
 *    at 0x8079CFF8/0x8079CFFC, and one translation unit pools a value once.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. AXFXReverbHiInit/Shutdown/Callback are the dump's names; GUESS: AXFXReverbHiSettings.  The map's Hi/Std
 *    labels are kept; see `AX/AXFXReverbStd.c` for the open question of which body is the Hi one.
 * RESIDUALS. none measured.
 * SHAPES. float constants are literals (retail loads each from the pool at its use).
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbHiExp.h"

/* Loads a reverb unit's user parameters into the live block and re-initialises the delay lines. */
BOOL AXFXReverbHiInit(AXFXReverbHi* reverb)
{
    reverb->mode = 5;
    reverb->pre_delay = reverb->params.a.in_pre_delay;
    reverb->pre_delay2 = reverb->params.a.in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->params.a.in_time;
    reverb->damping = reverb->params.a.in_damping;
    reverb->coloration = reverb->params.a.in_coloration;
    reverb->crosstalk = reverb->params.a.in_crosstalk;
    reverb->out_gain = 0.0f;
    reverb->dry_gain = 1.0f;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->params.a.mix = reverb->params.a.in_mix;
    reverb->params.a.early_gain = 0.0f;
    return AXFXReverbHiExpInit(reverb);
}

/* Releases the reverb unit. */
BOOL AXFXReverbHiShutdown(AXFXReverbHi* reverb)
{
    DVDCancel(reverb);
    return TRUE;
}

/* Loads the unit's user parameters and rebuilds the delay lines in place. */
BOOL AXFXReverbHiSettings(AXFXReverbHi* reverb)
{
    reverb->mode = 5;
    reverb->pre_delay = reverb->params.a.in_pre_delay;
    reverb->pre_delay2 = reverb->params.a.in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->params.a.in_time;
    reverb->damping = reverb->params.a.in_damping;
    reverb->coloration = reverb->params.a.in_coloration;
    reverb->crosstalk = reverb->params.a.in_crosstalk;
    reverb->out_gain = 0.0f;
    reverb->dry_gain = 1.0f;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->params.a.mix = reverb->params.a.in_mix;
    reverb->params.a.early_gain = 0.0f;
    return AXFXReverbHiExpSettings(reverb);
}

/* The effect callback entry point. */
void AXFXReverbHiCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb)
{
    AXFXReverbHiExpCallback(buffer, reverb);
}
