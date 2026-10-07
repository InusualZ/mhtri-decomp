/*
 * AX/AXFXReverbHi.c - the AXFX reverb API wrappers: init, shutdown, settings and callback for the three-channel
 *    body (AXFXReverbHiExp.c) and for the four-channel body (AXFXReverbStd.cpp).
 *
 * RANGE. .text 0x80474CB0-0x80474F30 (8 functions, 0x220 B); no data of its own.  Right edge: the start of
 *    `AX/AXFXReverbHiExp.c`.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. AXFXReverbHiInit/Shutdown/Callback are the dump's names; the other wrappers follow them (GUESS:
 *    AXFXReverbHiSettings, AXFXReverbStdInit/Shutdown/Settings/Callback).  The sound unit `sound/fn_800E46E8.cpp`
 *    drives the four-channel body (AXGetMode() == 2 gate) as its "Hi" reverb, which suggests the map's Hi/Std
 *    labels of the two wrapper groups are swapped; the map names are kept.
 * RESIDUALS. the two groups are two translation units: the `.sdata2` pool 0x8079CFF0-0x8079D000 holds 0.0f/1.0f
 *    twice (once per group), the compiler here pools them once (8 B of 16 B); seam: 0x80474CB0..0x80474DD0 and
 *    0x80474DD0..0x80474F30.
 * SHAPES. float constants are literals (retail loads each from the pool at its use).
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbHiExp.h"
#include "AX/AXFXReverbStd.h"
#include "AX/AXCL.h"

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

/* The "Exp" unit's parameter load: only valid when the AX output mode is stereo. */
BOOL AXFXReverbStdInit(AXFXReverbStd* reverb)
{
    if (AXGetMode() != 2) {
        return FALSE;
    }
    reverb->mode = 5;
    reverb->pre_delay = reverb->in_pre_delay;
    reverb->pre_delay2 = reverb->in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->in_time;
    reverb->damping = reverb->in_damping;
    reverb->coloration = reverb->in_coloration;
    reverb->crosstalk = reverb->in_crosstalk;
    reverb->out_gain = 0.0f;
    reverb->dry_gain = 1.0f;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->mix = reverb->in_mix;
    reverb->early_gain = 0.0f;
    return AXFXReverbStdExpInit(reverb);
}

/* Releases the "Exp" unit. */
BOOL AXFXReverbStdShutdown(AXFXReverbStd* reverb)
{
    AXFXReverbStdExpShutdown(reverb);
    return TRUE;
}

/* Re-loads the "Exp" unit's user parameters and rebuilds its delay lines in place. */
BOOL AXFXReverbStdSettings(AXFXReverbStd* reverb)
{
    reverb->mode = 5;
    reverb->pre_delay = reverb->in_pre_delay;
    reverb->pre_delay2 = reverb->in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->in_time;
    reverb->damping = reverb->in_damping;
    reverb->coloration = reverb->in_coloration;
    reverb->crosstalk = reverb->in_crosstalk;
    reverb->out_gain = 0.0f;
    reverb->dry_gain = 1.0f;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->mix = reverb->in_mix;
    reverb->early_gain = 0.0f;
    return AXFXReverbStdExpSettings(reverb);
}

/* The "Exp" unit's callback entry point. */
void AXFXReverbStdCallback(AXFXBufferStd* buffer, AXFXReverbStd* reverb)
{
    AXFXReverbStdExpCallback(buffer, reverb);
}
