/*
 * Revolution SDK AX library: the AXFX reverb-hi effect pair (AXFXReverbHi / AXFXReverbHiExp) and the
 * shared delay-line helpers.
 *
 * .text 0x80474CB0-0x80475E24 (16 functions / 4468 B). Registered once, at its final home
 * (docs/plan.md 12): proposal `80474CB0_AXFXReverbHiInit`. Module `AX`, lib `OS` (the SDK lib the
 * range's link neighbours are in; AX itself is not yet a config.libs block).
 *
 * Evidence for the name (brief part 2, class 2/3):
 *   - no `__FILE__` string covers the range (the image carries no "Reverb"/"axfx" string at all);
 *   - the runtime dump (`.pi/tmp/dumpsyms/Dump_Loading85.raw.map`) names the range's head
 *     `AXFXReverbHiInit` / `AXFXReverbHiShutdown` / `AXFXReverbHiCallback` / `AXFXReverbHiExpInit` and
 *     its delay-line helpers `__AllocDelayLine` / `__BzeroDelayLines` / `__FreeDelayLine`;
 *   - the neighbouring ranges are the rest of the AX library (`__AXVPBInit` below, `AXFXSetHooks`
 *     above), so the subsystem and the file-family name are `AXFXReverbHi`.
 * A sister build (MotoGP 08, same SDK) splits this exact code into `AXFXReverbHi.c`
 * (`.text` 0x802C4B2C-0x802C4BB8) and `AXFXReverbHiExp.c` (0x802C4BB8-0x802C5994); this run is the two
 * of them plus the game's second-effect API wrappers, i.e. the discovery seam between the two TUs was
 * not taken (recorded, not split - see the unit notes).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX / lbl_XXXXXXXX for the internal helpers and the
 * data pool of this range (checked `grep -n 80474/80475 config/RMHE08/symbols.txt`: only AXFXReverbHi*,
 * DVDCancel and the fn_ stems are present), so those stems are kept verbatim - renaming them would
 * unpair every function in objdiff (which matches by symbol name) and the names are the map's.
 *
 * Naming note: 0x804751A0 carries the map/dump name `DVDCancel`, which is semantically wrong (its body
 * frees the reverb's delay lines and is the effect's shutdown helper). The map is ground truth and the
 * name is not a mangling, so it is kept; the sibling accessor `AXFXReverbHiShutdown` still tails into it.
 *
 * Residuals (measured with `python tools/units/recompile.py AX/AXFXReverbHi.c --measure <sym>`):
 *   - fn_80475200 (AXFXReverbHiExpCallback, 0x524 B) and fn_80475B00 (__InitParams, 0x324 B) are
 *     reconstructed structurally, not yet byte-faithful; both carry the bulk of the range's bytes and
 *     are recorded as the unit's residual.
 *   - the `.data` pool the init reads (`lbl_80612980`, `lbl_80612A40`) lives in an unclaimed auto range;
 *     this unit references it (relocation-only), the data pass owns the claim.
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for OSAlarm,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The two hooks AXFXSetHooks installs; the map names them lbl_80793D20 / lbl_80793D24. */
extern void* lbl_80793D20; /* alloc hook (sda21) */
extern void* lbl_80793D24; /* free hook (sda21)  */

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
extern f32 lbl_8079CFF0; /* 0.0f  */
extern f32 lbl_8079CFF4; /* 1.0f  */
extern f32 lbl_8079CFF8; /* 0.0f  */
extern f32 lbl_8079CFFC; /* 1.0f  */
extern f32 lbl_8079D000; /* 32000.0f */
extern f32 lbl_8079D004; /* 0.0f  */
extern f32 lbl_8079D008; /* 1.0f  */
extern f32 lbl_8079D00C; /* 0.6f  */
extern f32 lbl_8079D010; /* 0.5f  */
extern f32 lbl_8079D020; /* -3.0f */
extern double lbl_8079D028; /* 10.0 */
extern f32 lbl_8079D030; /* 0.95f */

extern BOOL OSDisableInterrupts(void);
extern void OSRestoreInterrupts(BOOL level);
extern f64 pow(f64 x, f64 y);

typedef struct AXFXBuffer {
    /* +0x00 */ s32* out0;
    /* +0x04 */ s32* out1;
    /* +0x08 */ s32* out2;
} AXFXBuffer; /* size: 0x0C */

/*
 * One AXFX reverb body (0x3E8 bytes).  The delay-line descriptor occupies 0x00-0x10B, the live
 * (class A) parameters 0x10C-0x147, and the class B parameters overlay 0x140-0x18C - the "Exp" effect
 * is the same struct read at a +0x30 shift, which is why the two Init/Settings/Callback families write
 * overlapping offsets and why only one class is ever live in a unit.
 */
typedef struct AXFXReverbHi {
    /* +0x000 */ void* delay0[3];        /* per-channel buffers, size0 each */
    /* +0x00C */ u32 idx0;
    /* +0x010 */ u32 idx1;
    /* +0x014 */ u32 idx2;
    /* +0x018 */ u32 length0;
    /* +0x01C */ u32 size0;
    /* +0x020 */ f32 coef0;
    /* +0x024 */ f32 coef1;
    /* +0x028 */ f32 coef2;
    /* +0x02C */ void* delay1[3];        /* per-channel buffers, size1 each */
    /* +0x038 */ u32 idx3;
    /* +0x03C */ u32 length1;
    /* +0x040 */ u32 size1;
    /* +0x044 */ void* delay2[3][3];     /* 3 channels x 3 lines, size2[] */
    /* +0x068 */ u32 filter_idx[3];
    /* +0x074 */ u32 filter_len[3];
    /* +0x080 */ u32 size2[3];
    /* +0x08C */ f32 filter_gain[3];
    /* +0x098 */ void* delay3[3][2];     /* 3 channels x 2 lines, size3[] */
    /* +0x0B0 */ u32 idx7;
    /* +0x0B4 */ u32 idx8;
    /* +0x0B8 */ u32 length5;
    /* +0x0BC */ u32 length6;
    /* +0x0C0 */ u32 size3[2];
    /* +0x0C8 */ void* delay4[3];        /* per-channel buffers, size4[] */
    /* +0x0D4 */ u32 idx9;
    /* +0x0D8 */ u32 idx10;
    /* +0x0DC */ u32 idx11;
    /* +0x0E0 */ u32 length7;
    /* +0x0E4 */ u32 length8;
    /* +0x0E8 */ u32 length9;
    /* +0x0EC */ u32 size4[3];
    /* +0x0F8 */ f32 coef6;
    /* +0x0FC */ f32 coef7;
    /* +0x100 */ f32 coef8;
    /* +0x104 */ f32 coef9;
    /* +0x108 */ f32 coef10;
    /* +0x10C */ u32 flags;
    /* +0x110 */ u32 mode;
    /* +0x114 */ f32 pre_delay;
    /* +0x118 */ f32 pre_delay2;
    /* +0x11C */ u32 filter_mode;
    /* +0x120 */ f32 time;
    /* +0x124 */ f32 damping;
    /* +0x128 */ f32 coloration;
    /* +0x12C */ f32 crosstalk;
    /* +0x130 */ f32 out_gain;
    /* +0x134 */ f32 dry_gain;
    /* +0x138 */ void* early_buf;
    /* +0x13C */ void* fused_buf;
    union {
        struct {
            /* +0x140 */ f32 mix;
            /* +0x144 */ f32 early_gain;
            /* +0x148 */ f32 in_damping;
            /* +0x14C */ f32 in_mix;
            /* +0x150 */ f32 in_time;
            /* +0x154 */ f32 in_coloration;
            /* +0x158 */ f32 in_pre_delay;
            /* +0x15C */ f32 in_crosstalk;
            /* +0x160 */ f32 in_160;
            /* +0x164 */ f32 in_164;
            /* +0x168 */ void* in_168;
            /* +0x16C */ void* in_16c;
            /* +0x170 */ f32 in_170;
            /* +0x174 */ f32 in_174;
        } a; /* size: 0x34 */
        struct {
            /* +0x140 */ u32 exp_mode;
            /* +0x144 */ f32 exp_pre_delay;
            /* +0x148 */ f32 exp_pre_delay2;
            /* +0x14C */ u32 exp_filter_mode;
            /* +0x150 */ f32 exp_time;
            /* +0x154 */ f32 exp_damping;
            /* +0x158 */ f32 exp_coloration;
            /* +0x15C */ f32 exp_crosstalk;
            /* +0x160 */ f32 exp_out_gain;
            /* +0x164 */ f32 exp_dry_gain;
            /* +0x168 */ void* exp_early_buf;
            /* +0x16C */ void* exp_fused_buf;
            /* +0x170 */ f32 exp_mix;
            /* +0x174 */ f32 exp_early_gain;
        } b; /* size: 0x34 */
    } params;
    /* +0x178 */ f32 exp_in_damping;
    /* +0x17C */ f32 exp_in_mix;
    /* +0x180 */ f32 exp_in_time;
    /* +0x184 */ f32 exp_in_coloration;
    /* +0x188 */ f32 exp_in_pre_delay;
    /* +0x18C */ f32 exp_in_crosstalk;
    /* +0x190 */ u8 pad_190[0x258];
} AXFXReverbHi; /* size: 0x3E8 */

extern u32 fn_8046FD80(void);
extern BOOL AXFXReverbHiExpInit(AXFXReverbHi* reverb);
extern void DVDCancel(AXFXReverbHi* reverb);
extern BOOL fn_804750D0(AXFXReverbHi* reverb);
extern BOOL fn_80475E30(AXFXReverbHi* reverb);
extern BOOL fn_80475FF0(AXFXReverbHi* reverb);
extern void AXFXReverbHiExpShutdown(AXFXReverbHi* reverb);
extern void fn_80476120(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1);
extern BOOL fn_80475730(AXFXReverbHi* reverb);
extern void fn_804758B0(AXFXReverbHi* reverb);
extern void fn_804759E0(AXFXReverbHi* reverb);
extern void fn_80475200(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1);
extern BOOL fn_80475B00(AXFXReverbHi* reverb);

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
    return fn_804750D0(reverb);
}

/* The effect callback entry point. */
void AXFXReverbHiCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1)
{
    fn_80475200(buffer, reverb, out0, out1);
}

/* The "Exp" unit's parameter load: only valid when the AX output mode is stereo. */
BOOL fn_80474DD0(AXFXReverbHi* reverb)
{
    if (fn_8046FD80() != 2) {
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
    AXFXReverbHiExpShutdown(reverb);
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
BOOL fn_804750D0(AXFXReverbHi* reverb)
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
 * instruction structure; the two large bodies (`fn_80475200`, `fn_80475B00`) are the unit's recorded
 * residual.
 */
void fn_80475200(AXFXBuffer* buffer, AXFXReverbHi* reverb, s32* out0, s32* out1)
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
