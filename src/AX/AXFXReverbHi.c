/*
 * Revolution SDK AX library: the AXFX reverb API wrappers, `.text` 0x80474CB0..0x80474F30 (AXFXReverbHiInit,
 * AXFXReverbHiShutdown, AXFXReverbHiCallback, their "Exp"-layout twins and the callback thunks).
 *
 * Phase 4 recut of the former single unit 0x80474CB0..0x80475E24: the tail from 0x80474F30 is now
 * AX/AXFXReverbHiExp.c (a sister build, MotoGP 08, splits the code at the same address).
 *
 * Evidence for the names: the runtime dump (`.pi/tmp/dumpsyms/Dump_Loading85.raw.map`) names AXFXReverbHiInit /
 * AXFXReverbHiShutdown / AXFXReverbHiCallback; the other functions keep the map's `fn_` stems.  The AXFXReverbHi
 * layout is in AX/AXFXReverbHi.h, the callees' declarations are in the headers of the units that own them.
 *
 * Residuals: the callees `AXFXReverbHiExpCallback` and `fn_80475B00` (AXFXReverbHiExp.c) are structural reconstructions, not
 * byte-faithful; `-O4,p`'s 16-byte function alignment is restored below.  Last measured 2026-09: see the objdiff report.
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbHiExp.h"
#include "AX/AXFXReverbStd.h"
#include "AX/AXCL.h"

/* The .sdata2 float pool the range's relocations point at (auto range 0x8079CFF0-0x8079D01F). */
extern f32 lbl_8079CFF0; /* 0.0f  */
extern f32 lbl_8079CFF4; /* 1.0f  */
extern f32 lbl_8079CFF8; /* 0.0f  */
extern f32 lbl_8079CFFC; /* 1.0f  */

/* Loads a reverb unit's user parameters into the live block and re-initialises the delay lines. */
BOOL AXFXReverbHiInit(AXFXReverbHi* reverb)
{
    f32 zero = lbl_8079CFF0;
    f32 one = lbl_8079CFF4;

    reverb->mode = 5;
    reverb->pre_delay = reverb->params.a.in_pre_delay;
    reverb->pre_delay2 = reverb->params.a.in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->params.a.in_time;
    reverb->damping = reverb->params.a.in_damping;
    reverb->coloration = reverb->params.a.in_coloration;
    reverb->crosstalk = reverb->params.a.in_crosstalk;
    reverb->out_gain = zero;
    reverb->dry_gain = one;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->params.a.mix = reverb->params.a.in_mix;
    reverb->params.a.early_gain = zero;
    return AXFXReverbHiExpInit(reverb);
}

/* Releases the reverb unit. */
BOOL AXFXReverbHiShutdown(AXFXReverbHi* reverb)
{
    DVDCancel(reverb);
    return TRUE;
}

/* Loads the unit's user parameters and rebuilds the delay lines in place. */
BOOL fn_80474D50(AXFXReverbHi* reverb)
{
    f32 zero = lbl_8079CFF0;
    f32 one = lbl_8079CFF4;

    reverb->mode = 5;
    reverb->pre_delay = reverb->params.a.in_pre_delay;
    reverb->pre_delay2 = reverb->params.a.in_pre_delay;
    reverb->filter_mode = 0;
    reverb->time = reverb->params.a.in_time;
    reverb->damping = reverb->params.a.in_damping;
    reverb->coloration = reverb->params.a.in_coloration;
    reverb->crosstalk = reverb->params.a.in_crosstalk;
    reverb->out_gain = zero;
    reverb->dry_gain = one;
    reverb->early_buf = 0;
    reverb->fused_buf = 0;
    reverb->params.a.mix = reverb->params.a.in_mix;
    reverb->params.a.early_gain = zero;
    return AXFXReverbHiExpSettings(reverb);
}

/* The effect callback entry point. */
void AXFXReverbHiCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1)
{
    AXFXReverbHiExpCallback(buffer, reverb, out0, out1);
}

/* The "Exp" unit's parameter load: only valid when the AX output mode is stereo. */
BOOL fn_80474DD0(AXFXReverbHi* reverb)
{
    if (AXGetMode() != 2) {
        return FALSE;
    }
    reverb->params.b.exp_mode = 5;
    reverb->params.b.exp_pre_delay = reverb->exp_in_pre_delay;
    reverb->params.b.exp_pre_delay2 = reverb->exp_in_pre_delay;
    reverb->params.b.exp_filter_mode = 0;
    reverb->params.b.exp_time = reverb->exp_in_time;
    reverb->params.b.exp_damping = reverb->exp_in_damping;
    reverb->params.b.exp_coloration = reverb->exp_in_coloration;
    reverb->params.b.exp_crosstalk = reverb->exp_in_crosstalk;
    reverb->params.b.exp_out_gain = lbl_8079CFF8;
    reverb->params.b.exp_dry_gain = lbl_8079CFFC;
    reverb->params.b.exp_early_buf = 0;
    reverb->params.b.exp_fused_buf = 0;
    reverb->params.b.exp_mix = reverb->exp_in_mix;
    reverb->params.b.exp_early_gain = lbl_8079CFF8;
    return fn_80475E30(reverb);
}

/* Releases the "Exp" unit. */
BOOL fn_80474E80(AXFXReverbHi* reverb)
{
    AXFXReverbStdShutdown(reverb);
    return TRUE;
}

/* Re-loads the "Exp" unit's user parameters and rebuilds its delay lines in place. */
BOOL fn_80474EB0(AXFXReverbHi* reverb)
{
    f32 zero = lbl_8079CFF8;
    f32 one = lbl_8079CFFC;

    reverb->params.b.exp_mode = 5;
    reverb->params.b.exp_pre_delay = reverb->exp_in_pre_delay;
    reverb->params.b.exp_pre_delay2 = reverb->exp_in_pre_delay;
    reverb->params.b.exp_filter_mode = 0;
    reverb->params.b.exp_time = reverb->exp_in_time;
    reverb->params.b.exp_damping = reverb->exp_in_damping;
    reverb->params.b.exp_coloration = reverb->exp_in_coloration;
    reverb->params.b.exp_crosstalk = reverb->exp_in_crosstalk;
    reverb->params.b.exp_out_gain = zero;
    reverb->params.b.exp_dry_gain = one;
    reverb->params.b.exp_early_buf = 0;
    reverb->params.b.exp_fused_buf = 0;
    reverb->params.b.exp_mix = reverb->exp_in_mix;
    reverb->params.b.exp_early_gain = zero;
    return fn_80475FF0(reverb);
}

/* The "Exp" unit's callback entry point. */
void fn_80474F20(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1)
{
    fn_80476120(buffer, reverb, out0, out1);
}
