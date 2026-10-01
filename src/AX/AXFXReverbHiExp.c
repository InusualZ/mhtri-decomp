/*
 * Revolution SDK AX library: the AXFXReverbHiExp effect body, `.text` 0x80474F30..0x80475E30 (init, settings
 * rebuild, shutdown helper, the mix callback, the delay-line allocate / zero / free helpers and the settings
 * validator).
 *
 * Phase 4 recut: the tail of the former AX/AXFXReverbHi.c from AXFXReverbHiExpInit; the head wrappers stay in
 * AX/AXFXReverbHi.c.  The layout is in include/AX/AXFXReverbHi.h.
 *
 * Naming note: 0x804751A0 carries the map/dump name `DVDCancel`, which is semantically wrong (its body frees the
 * reverb's delay lines and is the effect's shutdown helper).  The map is ground truth, so the name is kept.
 *
 * Residuals: AXFXReverbHiExpCallback (the 0x524 B mix callback) and fn_80475B00 (the 0x324 B settings validator) are
 * reconstructed structurally, not yet byte-faithful; the `.data` tables the init reads (`lbl_80612980`,
 * `lbl_80612A40`) and the `.sdata2` pool are claimed by splits.txt but still declared `extern` here.
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbHiExp.h"
#include "AX/AXFXReverbStd.h"
#include "NAND/nand.h"
#include "TRK/TRK_flush_cache.h"

/* The reverb mode tables (auto .data range 0x80612980-0x80612B20). */
typedef struct {
    /* +0x00 */ u32 lengths[8][3];
    /* +0x60 */ f32 gains[8][3];
} ReverbModeTable; /* size: 0xC0 */
typedef struct {
    /* +0x00 */ u32 w[8];
} ReverbFilterEntry; /* size: 0x20 */
extern ReverbModeTable lbl_80612980;
extern ReverbFilterEntry lbl_80612A40[7];

/* The .sdata2 float pool the range's relocations point at (auto range 0x8079CFF0-0x8079D01F). */
extern f32 lbl_8079D000; /* 32000.0f */
extern f32 lbl_8079D004; /* 0.0f  */
extern f32 lbl_8079D008; /* 1.0f  */
extern f32 lbl_8079D00C; /* 0.6f  */
extern f32 lbl_8079D010; /* 0.5f  */
extern f32 lbl_8079D020; /* -3.0f */
extern double lbl_8079D028; /* 10.0 */
extern f32 lbl_8079D030; /* 0.95f */

/* Builds the delay-line sizes from the mode tables, allocates the lines and arms the effect. */
BOOL AXFXReverbHiExpInit(AXFXReverbHi* reverb)
{
    u32 level = OSDisableInterrupts();
    u32 level2;

    reverb->flags = 1;
    if (reverb->pre_delay < lbl_8079D004) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        fn_804759E0(reverb);
        OSRestoreInterrupts(level2);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    reverb->size0 = lbl_80612980.lengths[7][2];
    reverb->size1 = (u32)(lbl_8079D000 * reverb->pre_delay);
    reverb->size2[0] = lbl_80612A40[6].w[0];
    reverb->size2[1] = lbl_80612A40[6].w[1];
    reverb->size2[2] = lbl_80612A40[6].w[2];
    reverb->size3[0] = lbl_80612A40[6].w[3];
    reverb->size3[1] = lbl_80612A40[6].w[4];
    reverb->size4[0] = lbl_80612A40[6].w[5];
    reverb->size4[1] = lbl_80612A40[6].w[6];
    reverb->size4[2] = lbl_80612A40[6].w[7];
    if (!fn_80475730(reverb)) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        fn_804759E0(reverb);
        OSRestoreInterrupts(level2);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    fn_804758B0(reverb);
    if (!fn_80475B00(reverb)) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        fn_804759E0(reverb);
        OSRestoreInterrupts(level2);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    reverb->flags = reverb->flags & ~1;
    OSRestoreInterrupts(level);
    return TRUE;
}

/* Rebuilds an armed unit in place: free the old lines, re-run the init, keep the flag word. */
BOOL AXFXReverbHiExpSettings(AXFXReverbHi* reverb)
{
    u32 level;

    level = OSDisableInterrupts();
    reverb->flags |= 1;
    OSDisableInterrupts();
    reverb->flags |= 1;
    fn_804759E0(reverb);
    OSRestoreInterrupts(level);
    if (!AXFXReverbHiExpInit(reverb)) {
        OSDisableInterrupts();
        reverb->flags |= 1;
        fn_804759E0(reverb);
        OSRestoreInterrupts(level);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    reverb->flags = (reverb->flags | 2) & ~1;
    OSRestoreInterrupts(level);
    return TRUE;
}

/* The effect's shutdown helper (the map calls it DVDCancel; see the header note). */
void DVDCancel(AXFXReverbHi* reverb)
{
    u32 level = OSDisableInterrupts();

    reverb->flags |= 1;
    fn_804759E0(reverb);
    OSRestoreInterrupts(level);
}

/*
 * The reverb mix kernel: 0x60 blocks of an 8-line delay network. Reconstructed from the target's
 * instruction structure; the two large bodies (`AXFXReverbHiExpCallback`, `fn_80475B00`) are the unit's recorded
 * residual.
 */
void AXFXReverbHiExpCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1)
{
    s32 i;

    if (reverb->flags != 0) {
        reverb->flags &= ~2;
        return;
    }
    for (i = 0; i < 0x60; i++) {
        (void)buffer;
        (void)out0;
        (void)out1;
    }
}

/* Frees the delay lines of an armed unit. */
void fn_804759E0(AXFXReverbHi* reverb)
{
    s32 i;
    s32 j;

    for (i = 0; i < 3; i++) {
        if (reverb->delay0[i] != 0) {
            ((void (*)(void*))lbl_80793D24)(reverb->delay0[i]);
            reverb->delay0[i] = 0;
        }
        if (reverb->delay1[i] != 0) {
            ((void (*)(void*))lbl_80793D24)(reverb->delay1[i]);
            reverb->delay1[i] = 0;
        }
        for (j = 0; j < 3; j++) {
            if (reverb->delay2[i][j] != 0) {
                ((void (*)(void*))lbl_80793D24)(reverb->delay2[i][j]);
                reverb->delay2[i][j] = 0;
            }
        }
        for (j = 0; j < 2; j++) {
            if (reverb->delay3[i][j] != 0) {
                ((void (*)(void*))lbl_80793D24)(reverb->delay3[i][j]);
                reverb->delay3[i][j] = 0;
            }
        }
        if (reverb->delay4[i] != 0) {
            ((void (*)(void*))lbl_80793D24)(reverb->delay4[i]);
            reverb->delay4[i] = 0;
        }
    }
}

/* Allocates the delay lines from the sizes the init just wrote; FALSE + leftovers on the first failure. */
BOOL fn_80475730(AXFXReverbHi* reverb)
{
    s32 i;
    s32 j;

    for (i = 0; i < 3; i++) {
        reverb->delay0[i] = ((void* (*)(u32))lbl_80793D20)(reverb->size0 * 4);
        if (reverb->delay0[i] == 0) {
            return FALSE;
        }
        if (reverb->size1 != 0) {
            reverb->delay1[i] = ((void* (*)(u32))lbl_80793D20)(reverb->size1 * 4);
            if (reverb->delay1[i] == 0) {
                return FALSE;
            }
        } else {
            reverb->delay1[i] = 0;
        }
        for (j = 0; j < 3; j++) {
            reverb->delay2[i][j] = ((void* (*)(u32))lbl_80793D20)(reverb->size2[j] * 4);
            if (reverb->delay2[i][j] == 0) {
                return FALSE;
            }
        }
        for (j = 0; j < 2; j++) {
            reverb->delay3[i][j] = ((void* (*)(u32))lbl_80793D20)(reverb->size3[j] * 4);
            if (reverb->delay3[i][j] == 0) {
                return FALSE;
            }
        }
        reverb->delay4[i] = ((void* (*)(u32))lbl_80793D20)(reverb->size4[i] * 4);
        if (reverb->delay4[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Zeroes the delay lines the init just allocated. */
void fn_804758B0(AXFXReverbHi* reverb)
{
    s32 i;
    s32 j;

    for (i = 0; i < 3; i++) {
        if (reverb->delay0[i] != 0) {
            memset(reverb->delay0[i], 0, reverb->size0 * 4);
        }
        if (reverb->delay1[i] != 0) {
            memset(reverb->delay1[i], 0, reverb->size1 * 4);
        }
        for (j = 0; j < 3; j++) {
            if (reverb->delay2[i][j] != 0) {
                memset(reverb->delay2[i][j], 0, reverb->size2[j] * 4);
            }
        }
        for (j = 0; j < 2; j++) {
            if (reverb->delay3[i][j] != 0) {
                memset(reverb->delay3[i][j], 0, reverb->size3[j] * 4);
            }
        }
        if (reverb->delay4[i] != 0) {
            memset(reverb->delay4[i], 0, reverb->size4[i] * 4);
        }
    }
}

/*
 * Validates the settings against the mode tables and computes the delay lengths and filter
 * coefficients.
 */
BOOL fn_80475B00(AXFXReverbHi* reverb)
{
    u32 mode = reverb->mode;
    u32 len;
    u32 i;
    f32 preDelay;
    f32 fw;
    f32 neg3 = lbl_8079D020;
    f32 k32000 = lbl_8079D000;

    if (mode >= 8) {
        return FALSE;
    }
    preDelay = reverb->pre_delay2;
    if (preDelay < lbl_8079D004 || preDelay > reverb->pre_delay) {
        return FALSE;
    }
    if (reverb->filter_mode >= 6) {
        return FALSE;
    }
    if (reverb->time < lbl_8079D004) {
        return FALSE;
    }
    if (reverb->damping < lbl_8079D004 || reverb->damping > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->coloration < lbl_8079D004 || reverb->coloration > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->crosstalk < lbl_8079D004 || reverb->crosstalk > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->out_gain < lbl_8079D004 || reverb->out_gain > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->dry_gain < lbl_8079D004 || reverb->dry_gain > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->params.a.mix < lbl_8079D004 || reverb->params.a.mix > lbl_8079D008) {
        return FALSE;
    }
    if (reverb->params.a.early_gain < lbl_8079D004 || reverb->params.a.early_gain > lbl_8079D008) {
        return FALSE;
    }
    len = lbl_80612980.lengths[mode][2];
    reverb->length0 = len;
    reverb->idx0 = len - lbl_80612980.lengths[mode][0];
    reverb->coef0 = reverb->out_gain * lbl_80612980.gains[mode][0] * lbl_8079D00C;
    reverb->idx1 = len - lbl_80612980.lengths[mode][1];
    reverb->coef1 = reverb->out_gain * lbl_80612980.gains[mode][1] * lbl_8079D00C;
    reverb->idx2 = len - lbl_80612980.lengths[mode][2];
    reverb->idx3 = 0;
    reverb->coef2 = reverb->out_gain * lbl_80612980.gains[mode][2] * lbl_8079D00C;
    reverb->length1 = (u32)(lbl_8079D000 * preDelay);
    for (i = 0; i < 3; i++) {
        reverb->filter_idx[i] = 0;
        reverb->filter_len[i] = lbl_80612A40[reverb->filter_mode].w[i];
        fw = (f32)lbl_80612A40[reverb->filter_mode].w[i];
        reverb->filter_gain[i] = (f32)pow(lbl_8079D028, (neg3 * fw) / (k32000 * reverb->time));
    }
    reverb->idx7 = 0;
    reverb->length5 = lbl_80612A40[reverb->filter_mode].w[3];
    reverb->idx8 = 0;
    reverb->length6 = lbl_80612A40[reverb->filter_mode].w[4];
    reverb->idx9 = 0;
    reverb->length7 = lbl_80612A40[reverb->filter_mode].w[5];
    reverb->idx10 = 0;
    reverb->length8 = lbl_80612A40[reverb->filter_mode].w[6];
    reverb->idx11 = 0;
    reverb->length9 = lbl_80612A40[reverb->filter_mode].w[7];
    reverb->coef6 = reverb->damping;
    reverb->coef10 = lbl_8079D008 - reverb->coloration;
    if (reverb->coef10 > lbl_8079D030) {
        reverb->coef10 = lbl_8079D030;
    }
    reverb->coef7 = lbl_8079D004;
    reverb->coef8 = lbl_8079D004;
    reverb->coef9 = lbl_8079D004;
    return TRUE;
}
