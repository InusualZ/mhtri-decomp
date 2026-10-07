/*
 * AX/AXFXReverbHiExp.c - the AXFX three-channel reverb body: init, settings rebuild, shutdown, the 96-sample mix
 *    callback, the delay-line allocate / clear / free helpers and the settings validator.
 *
 * RANGE. .text 0x80474F30-0x80475E30 (8 functions, 0x3A5C B of rows); .data 0x80612980-0x80612B20; .sdata2
 *    0x8079D000-0x8079D040.  Left edge: the end of the `AXFXReverbHi.c` wrapper run; right edge: the start of
 *    `AX/AXFXReverbStdExp.cpp` (0x80475E30).  Layout of the body: AX/AXFXReverbHi.h.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below (every start is 16-aligned);
 *    `#pragma fp_contract off` (retail keeps `fmuls`+`fadds` in the mix kernel); loop counters are `u32`
 *    (retail compares with `cmplwi`).
 * NAMES. `DVDCancel` (0x804751A0) is the map's name for the shutdown helper whose body frees the delay lines (kept:
 *    the map is the authority); the Alloc/Clear/Free/ApplySettings rows are GUESSES from what each body does; the
 *    three tables are file statics named after their rows (sHiExpModeLength / sHiExpModeGain / sHiExpFilterLength).
 * RESIDUALS. AXFXReverbHiExpCallback: constant-register numbering and the position of the `0.0f` load of the comb
 *    accumulator differ (r7..r12 vs r5..r10, loop-invariant `lfs` order); AXFXReverbHiExpApplySettings: GPR
 *    numbering of the mode-table row pointer (r6/r7/r9); `.sdata2` 56 of 64 B: retail orders the pool 32000.0f
 *    before 0.0f and -3.0f before 10.0 (the unit's literal order cannot be reached from the source order tried);
 *    `.text` pads are linker-added.
 * SHAPES. the comb accumulator is a loop over j (retail unrolls it); the early/fused buffers are `AXFXBuffer`
 *    pointer triples copied to locals before the sample loop.
 */

/* -O4,p is this lib section's default function alignment (16); cflags_os overrides it to 4 for the OS units,
 * but every start in this range is 16-byte aligned (the 0xC gap between bodies), so restore it. */
#pragma function_align 16
#pragma fp_contract off

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "AX/AXFXReverbHi.h"
#include "AX/AXFXReverbHiExp.h"
#include "AX/AXFXHooks.h"
#include "NAND/nand.h"
#include "TRK/TRK_flush_cache.h"

/* Per-mode delay-line lengths (taps 0..2 of the early network). */
static u32 sHiExpModeLength[8][3] = {
    {157, 479, 829},
    {317, 809, 1117},
    {479, 941, 1487},
    {641, 1259, 1949},
    {797, 1667, 2579},
    {967, 1901, 2903},
    {1123, 2179, 3413},
    {1279, 2477, 3889},
};

/* Per-mode tap gains. */
static f32 sHiExpModeGain[8][3] = {
    {0.4f, -1.0f, 0.3f},
    {0.5f, -0.95f, 0.3f},
    {0.6f, -0.9f, 0.3f},
    {0.75f, -0.85f, 0.3f},
    {-0.9f, 0.8f, 0.3f},
    {-1.0f, 0.7f, 0.3f},
    {-1.0f, 0.7f, 0.3f},
    {-1.0f, 0.7f, 0.3f},
};

/* Per-filter-mode comb/all-pass lengths: three comb lines, two all-pass lines, three output lines. */
static u32 sHiExpFilterLength[7][8] = {
    {1789, 1999, 2333, 433, 149, 47, 73, 67},
    {149, 293, 449, 251, 103, 47, 73, 67},
    {947, 1361, 1531, 433, 137, 47, 73, 67},
    {1279, 1531, 1973, 509, 149, 47, 73, 67},
    {1531, 1847, 2297, 563, 179, 47, 73, 67},
    {1823, 2357, 2693, 571, 137, 47, 73, 67},
    {1823, 2357, 2693, 571, 179, 47, 73, 67},
};

/* Marks the unit disabled and frees its delay lines with interrupts off. */
static inline void ReverbHiExpFreeLocked(AXFXReverbHi* reverb)
{
    u32 level = OSDisableInterrupts();

    reverb->flags |= 1;
    AXFXReverbHiExpFreeLines(reverb);
    OSRestoreInterrupts(level);
}

/* Builds the delay-line sizes from the mode tables, allocates the lines and arms the effect. */
BOOL AXFXReverbHiExpInit(AXFXReverbHi* reverb)
{
    u32 level = OSDisableInterrupts();
    u32 level2;

    reverb->flags = 1;
    if (reverb->pre_delay < 0.0f) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        AXFXReverbHiExpFreeLines(reverb);
        OSRestoreInterrupts(level2);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    reverb->size0 = sHiExpModeLength[7][2];
    reverb->size1 = (u32)(32000.0f * reverb->pre_delay);
    reverb->size2[0] = sHiExpFilterLength[6][0];
    reverb->size2[1] = sHiExpFilterLength[6][1];
    reverb->size2[2] = sHiExpFilterLength[6][2];
    reverb->size3[0] = sHiExpFilterLength[6][3];
    reverb->size3[1] = sHiExpFilterLength[6][4];
    reverb->size4[0] = sHiExpFilterLength[6][5];
    reverb->size4[1] = sHiExpFilterLength[6][6];
    reverb->size4[2] = sHiExpFilterLength[6][7];
    if (!AXFXReverbHiExpAllocLines(reverb)) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        AXFXReverbHiExpFreeLines(reverb);
        OSRestoreInterrupts(level2);
        OSRestoreInterrupts(level);
        return FALSE;
    }
    AXFXReverbHiExpClearLines(reverb);
    if (!AXFXReverbHiExpApplySettings(reverb)) {
        level2 = OSDisableInterrupts();
        reverb->flags |= 1;
        AXFXReverbHiExpFreeLines(reverb);
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
    ReverbHiExpFreeLocked(reverb);
    if (!AXFXReverbHiExpInit(reverb)) {
        ReverbHiExpFreeLocked(reverb);
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
    AXFXReverbHiExpFreeLines(reverb);
    OSRestoreInterrupts(level);
}

/* Runs one 96-sample block of the three-channel reverb network over the buffers in place. */
void AXFXReverbHiExpCallback(AXFXBuffer* buffer, AXFXReverbHi* reverb)
{
    s32* chan[3];
    f32 res[3];
    f32 out[3];
    s32* early[3];
    s32* fused0;
    s32* fused1;
    s32* fused2;
    f32 dry;
    f32 mixRate;
    f32 damp;
    f32 lpk;
    f32 lpkInv;
    u32 i;
    u32 c;
    u32 j;

    if (reverb->flags != 0) {
        reverb->flags &= ~2;
        return;
    }
    chan[0] = buffer->out0;
    chan[1] = buffer->out1;
    chan[2] = buffer->out2;
    if (reverb->early_buf != 0) {
        early[0] = reverb->early_buf->out0;
        early[1] = reverb->early_buf->out1;
        early[2] = reverb->early_buf->out2;
    }
    if (reverb->fused_buf != 0) {
        fused0 = reverb->fused_buf->out0;
        fused1 = reverb->fused_buf->out1;
        fused2 = reverb->fused_buf->out2;
    }
    lpk = reverb->coef10;
    lpkInv = 1.0f - lpk;
    dry = 0.6f * reverb->dry_gain;
    mixRate = 0.5f * reverb->crosstalk;
    damp = reverb->coef6;
    for (i = 0; i < 96; i++) {
        for (c = 0; c < 3; c++) {
            f32 in;
            f32* line;
            f32 early_sum;
            f32 delayed;
            f32 comb;
            f32 a;
            f32 b;
            f32 lp;
            f32 d;
            f32 tail;

            if (reverb->early_buf != 0) {
                in = (f32)(*early[c]++ + *chan[c]);
            } else {
                in = (f32)*chan[c];
            }
            line = (f32*)reverb->delay0[c];
            early_sum = reverb->coef0 * line[reverb->idx0] + reverb->coef1 * line[reverb->idx1];
            a = line[reverb->idx2];
            line[reverb->idx2] = in;
            early_sum = reverb->coef2 * a + early_sum;
            if (reverb->length1 != 0) {
                line = (f32*)reverb->delay1[c];
                delayed = line[reverb->idx3];
                line[reverb->idx3] = in;
            } else {
                delayed = in;
            }
            comb = 0.0f;
            for (j = 0; j < 3; j++) {
                line = (f32*)reverb->delay2[c][j];
                a = line[reverb->filter_idx[j]];
                comb = comb + a;
                line[reverb->filter_idx[j]] = delayed + a * reverb->filter_gain[j];
            }
            line = (f32*)reverb->delay3[c][0];
            a = line[reverb->idx7];
            b = comb + a * damp;
            line[reverb->idx7] = b;
            a = a - b * damp;
            line = (f32*)reverb->delay3[c][1];
            b = line[reverb->idx8];
            d = a + b * damp;
            line[reverb->idx8] = d;
            a = b - d * damp;
            lp = lpkInv * a + lpk * reverb->coef7[c];
            reverb->coef7[c] = lp;
            line = (f32*)reverb->delay4[c];
            a = line[reverb->idx9[c]];
            tail = lp + a * damp;
            line[reverb->idx9[c]] = tail;
            reverb->idx9[c]++;
            out[c] = a - tail * damp;
            if (reverb->idx9[c] >= reverb->length7[c]) {
                reverb->idx9[c] = 0;
            }
            out[c] = early_sum + out[c] * dry;
        }
        {
            s32* p0 = chan[0];
            s32* p1 = chan[1];
            s32* p2 = chan[2];

            res[0] = out[0] + mixRate * (out[1] + out[2]);
            res[1] = out[1] + mixRate * (out[0] + out[2]);
            res[2] = out[2] + mixRate * (out[0] + out[1]);
            chan[0] = p0 + 1;
            chan[1] = p1 + 1;
            chan[2] = p2 + 1;
            *p0 = (s32)(res[0] * reverb->params.a.mix);
            *p1 = (s32)(res[1] * reverb->params.a.mix);
            *p2 = (s32)(res[2] * reverb->params.a.mix);
            if (reverb->fused_buf != 0) {
                *fused0++ = (s32)(res[0] * reverb->params.a.early_gain);
                *fused1++ = (s32)(res[1] * reverb->params.a.early_gain);
                *fused2++ = (s32)(res[2] * reverb->params.a.early_gain);
            }
        }
        if (++reverb->idx0 >= reverb->length0) {
            reverb->idx0 = 0;
        }
        if (++reverb->idx1 >= reverb->length0) {
            reverb->idx1 = 0;
        }
        if (++reverb->idx2 >= reverb->length0) {
            reverb->idx2 = 0;
        }
        if (reverb->length1 != 0) {
            if (++reverb->idx3 >= reverb->length1) {
                reverb->idx3 = 0;
            }
        }
        if (++reverb->filter_idx[0] >= reverb->filter_len[0]) {
            reverb->filter_idx[0] = 0;
        }
        if (++reverb->filter_idx[1] >= reverb->filter_len[1]) {
            reverb->filter_idx[1] = 0;
        }
        if (++reverb->filter_idx[2] >= reverb->filter_len[2]) {
            reverb->filter_idx[2] = 0;
        }
        if (++reverb->idx7 >= reverb->length5) {
            reverb->idx7 = 0;
        }
        if (++reverb->idx8 >= reverb->length6) {
            reverb->idx8 = 0;
        }
    }
}

/* Frees the delay lines of an armed unit. */
void AXFXReverbHiExpFreeLines(AXFXReverbHi* reverb)
{
    u32 i;
    u32 j;

    for (i = 0; i < 3; i++) {
        if (reverb->delay0[i] != 0) {
            AXFXFree(reverb->delay0[i]);
            reverb->delay0[i] = 0;
        }
        if (reverb->delay1[i] != 0) {
            AXFXFree(reverb->delay1[i]);
            reverb->delay1[i] = 0;
        }
        for (j = 0; j < 3; j++) {
            if (reverb->delay2[i][j] != 0) {
                AXFXFree(reverb->delay2[i][j]);
                reverb->delay2[i][j] = 0;
            }
        }
        for (j = 0; j < 2; j++) {
            if (reverb->delay3[i][j] != 0) {
                AXFXFree(reverb->delay3[i][j]);
                reverb->delay3[i][j] = 0;
            }
        }
        if (reverb->delay4[i] != 0) {
            AXFXFree(reverb->delay4[i]);
            reverb->delay4[i] = 0;
        }
    }
}

/* Allocates the delay lines from the sizes the init just wrote; FALSE + leftovers on the first failure. */
BOOL AXFXReverbHiExpAllocLines(AXFXReverbHi* reverb)
{
    u32 i;
    u32 j;

    for (i = 0; i < 3; i++) {
        reverb->delay0[i] = AXFXAlloc(reverb->size0 * 4);
        if (reverb->delay0[i] == 0) {
            return FALSE;
        }
        if (reverb->size1 != 0) {
            reverb->delay1[i] = AXFXAlloc(reverb->size1 * 4);
            if (reverb->delay1[i] == 0) {
                return FALSE;
            }
        } else {
            reverb->delay1[i] = 0;
        }
        for (j = 0; j < 3; j++) {
            reverb->delay2[i][j] = AXFXAlloc(reverb->size2[j] * 4);
            if (reverb->delay2[i][j] == 0) {
                return FALSE;
            }
        }
        for (j = 0; j < 2; j++) {
            reverb->delay3[i][j] = AXFXAlloc(reverb->size3[j] * 4);
            if (reverb->delay3[i][j] == 0) {
                return FALSE;
            }
        }
        reverb->delay4[i] = AXFXAlloc(reverb->size4[i] * 4);
        if (reverb->delay4[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Zeroes the delay lines the init just allocated. */
void AXFXReverbHiExpClearLines(AXFXReverbHi* reverb)
{
    u32 i;
    u32 j;

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
BOOL AXFXReverbHiExpApplySettings(AXFXReverbHi* reverb)
{
    u32 mode = reverb->mode;
    f32 preDelay;
    u32 len;
    u32 i;
    u32 tap;

    if (mode >= 8) {
        return FALSE;
    }
    preDelay = reverb->pre_delay2;
    if (preDelay < 0.0f || preDelay > reverb->pre_delay) {
        return FALSE;
    }
    if (reverb->filter_mode >= 6) {
        return FALSE;
    }
    if (reverb->time < 0.0f) {
        return FALSE;
    }
    if (reverb->damping < 0.0f || reverb->damping > 1.0f) {
        return FALSE;
    }
    if (reverb->coloration < 0.0f || reverb->coloration > 1.0f) {
        return FALSE;
    }
    if (reverb->crosstalk < 0.0f || reverb->crosstalk > 1.0f) {
        return FALSE;
    }
    if (reverb->out_gain < 0.0f || reverb->out_gain > 1.0f) {
        return FALSE;
    }
    if (reverb->dry_gain < 0.0f || reverb->dry_gain > 1.0f) {
        return FALSE;
    }
    if (reverb->params.a.mix < 0.0f || reverb->params.a.mix > 1.0f) {
        return FALSE;
    }
    if (reverb->params.a.early_gain < 0.0f || reverb->params.a.early_gain > 1.0f) {
        return FALSE;
    }
    len = sHiExpModeLength[mode][2];
    reverb->length0 = len;
    reverb->idx0 = len - sHiExpModeLength[mode][0];
    reverb->coef0 = reverb->out_gain * sHiExpModeGain[mode][0] * 0.6f;
    reverb->idx1 = len - sHiExpModeLength[mode][1];
    reverb->coef1 = reverb->out_gain * sHiExpModeGain[mode][1] * 0.6f;
    reverb->idx2 = len - sHiExpModeLength[mode][2];
    reverb->coef2 = reverb->out_gain * sHiExpModeGain[mode][2] * 0.6f;
    reverb->idx3 = 0;
    reverb->length1 = (u32)(32000.0f * preDelay);
    for (i = 0; i < 3; i++) {
        reverb->filter_idx[i] = 0;
        tap = sHiExpFilterLength[reverb->filter_mode][i];
        reverb->filter_len[i] = tap;
        reverb->filter_gain[i] = (f32)pow(10.0, (-3.0f * (f32)tap) / (32000.0f * reverb->time));
    }
    reverb->idx7 = 0;
    reverb->length5 = sHiExpFilterLength[reverb->filter_mode][3];
    reverb->idx8 = 0;
    reverb->length6 = sHiExpFilterLength[reverb->filter_mode][4];
    reverb->idx9[0] = 0;
    reverb->length7[0] = sHiExpFilterLength[reverb->filter_mode][5];
    reverb->idx9[1] = 0;
    reverb->length7[1] = sHiExpFilterLength[reverb->filter_mode][6];
    reverb->idx9[2] = 0;
    reverb->length7[2] = sHiExpFilterLength[reverb->filter_mode][7];
    reverb->coef6 = reverb->damping;
    reverb->coef10 = 1.0f - reverb->coloration;
    if (reverb->coef10 > 0.95f) {
        reverb->coef10 = 0.95f;
    }
    reverb->coef7[0] = 0.0f;
    reverb->coef7[1] = 0.0f;
    reverb->coef7[2] = 0.0f;
    return TRUE;
}
