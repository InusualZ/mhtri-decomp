/*
 * AX/AXFXReverbStd.c - the AXFX reverb API wrappers (init, shutdown, settings, callback) for the four-channel
 *    body in `AX/AXFXReverbStdExp.cpp`.
 *
 * RANGE. .text 0x80474DD0-0x80474F30 (4 functions, 0x160 B); .sdata2 0x8079CFF8-0x8079D000 (0.0f, 1.0f).  Left
 *    edge: the end of `AX/AXFXReverbHi.c`, proven by the pool (see there); right edge: the start of
 *    `AX/AXFXReverbHiExp.c`, proven by the pool as well (0.0f again at 0x8079D004).
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. GUESS: AXFXReverbStdInit/Shutdown/Settings/Callback.  The sound unit `sound/fn_800E46E8.cpp` drives this
 *    group (AXGetMode() == 2, Dolby Pro Logic II, four output channels) as its "Hi" reverb and types its data
 *    ReverbHiData, while the dump names the three-channel group Hi: the labels may be swapped.  Undecided - the
 *    map names are kept until a name for the four-channel body is evidenced.
 * RESIDUALS. none measured.
 * SHAPES. float constants are literals (retail loads each from the pool at its use).
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbStdExp.h"
#include "AX/AXCL.h"

/* The four-channel unit's parameter load: only valid in Dolby Pro Logic II output mode (AXGetMode() == 2). */
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

/* Releases the four-channel unit. */
BOOL AXFXReverbStdShutdown(AXFXReverbStd* reverb)
{
    AXFXReverbStdExpShutdown(reverb);
    return TRUE;
}

/* Re-loads the four-channel unit's user parameters and rebuilds its delay lines in place. */
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

/* The four-channel unit's callback entry point. */
void AXFXReverbStdCallback(AXFXBufferStd* buffer, AXFXReverbStd* reverb)
{
    AXFXReverbStdExpCallback(buffer, reverb);
}
