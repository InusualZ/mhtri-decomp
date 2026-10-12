/* WENC/wenc.cpp - the Wii remote speaker encoder (`WENCGetEncodeData`): 16-bit samples to 4-bit adaptive differential codes.
 * RANGE. .text 0x804EB22C-0x804EB510 (1 function); .rodata 0x80573C40-0x80573C80; .sdata2 0x8079D3C0-0x8079D3C8.
 *   Edges: its .rodata table and .sdata2 literal are read only by this function; both edges are 4-aligned (the
 *   function starts at 0x804EB22C, so the TU compiled without 16-byte function alignment, unlike its neighbours,
 *   whose starts are all 16-aligned).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`).
 * NAMES. `WENCGetEncodeData` is the map's name; the state record `WENCInfo` and its field names are read from the algorithm.
 * RESIDUALS. The loop tail: retail shares only `bit2 * 4` and `bit1 * 2` between the step-table index and the output nibble and
 *   truncates the nibble to a byte before the shift (728 bytes); this source shares the whole sum and truncates after it (720).
 *   Retail's tail keeps `bit2 * 4` in r18, which also shifts the registers of the prologue's 16-word `stepScale` copy.
 *   The .rodata table and .sdata2 literal match byte for byte but are emitted as the compiler-local `@18`/`@72`, where the map
 *   names them `lbl_80573C40`/`lbl_8079D3C0`. The target .text carries 12 bytes of tail padding up to WPAD's 16-aligned start
 *   (0x2E4) that this 4-aligned object does not emit.
 * SHAPES. The allocator follows the declaration order (`i`, `sign`, `bit2`..`bit0`, then the state); the bits are cleared
 *   `bit0` first; the absolute difference is `__abs`; the nibble shift is `(i & 1) == 0 ? 4 : 0`.
 */

#include "types.h"
#include "WENC/wenc.h"
#include "Runtime.PPCEABI.H/memset.h"

/* Encodes `sampleCount` samples into `out`, high nibble first; returns `sampleCount`. */
s32 WENCGetEncodeData(WENCInfo* info, u32 flags, const s16* samples, s32 sampleCount, u8* out)
{
    const f64 stepScale[8] = { 0.8984375, 0.8984375, 0.8984375, 0.8984375, 1.19921875, 1.59765625, 2.0, 2.3984375 };
    s32 i;
    s32 sign;
    s32 bit2;
    s32 bit1;
    s32 bit0;
    s32 predictor;
    s32 step;
    s32 delta;
    s32 remainder;
    s32 half;
    s32 quarter;

    memset(out, 0, (sampleCount + 1) / 2);
    if ((flags & 1) == 0) {
        predictor = 0;
        step = 0x7F;
        delta = 0;
        remainder = 0;
        half = 0;
        quarter = 0;
    } else {
        predictor = info->predictor;
        step = info->step;
        delta = info->delta;
        remainder = info->remainder;
        half = info->half;
        quarter = info->quarter;
    }
    for (i = 0; i < sampleCount; i++) {
        s16 sample = *samples++;

        bit0 = 0;
        bit1 = 0;
        bit2 = 0;
        sign = 0;
        if (sample < predictor) {
            sign = 1;
        }
        remainder = __abs(sample - predictor);
        if (remainder >= step) {
            bit2 = 1;
            remainder -= step;
        }
        half = step / 2;
        if (remainder >= half) {
            bit1 = 1;
            remainder -= half;
        }
        quarter = half / 2;
        if (remainder >= quarter) {
            bit0 = 1;
            remainder -= quarter;
        }
        delta = (1 - sign * 2) * (step * bit2 + half * bit1 + quarter * bit0 + quarter / 2);
        if (delta > 0xFFFF) {
            delta = 0xFFFF;
        }
        if (delta < -0x10000) {
            delta = -0x10000;
        }
        predictor += delta;
        if (predictor > 0x7FFF) {
            predictor = 0x7FFF;
        }
        if (predictor < -0x8000) {
            predictor = -0x8000;
        }
        out[i / 2] |= (sign * 8 + bit2 * 4 + bit1 * 2 + bit0) << ((i & 1) == 0 ? 4 : 0);
        step = (s32)((f64)step * stepScale[bit1 * 2 + bit0 + bit2 * 4]);
        if (step <= 0x7F) {
            step = 0x7F;
        }
        if (step >= 0x6000) {
            step = 0x6000;
        }
    }
    info->predictor = predictor;
    info->step = step;
    info->delta = delta;
    info->remainder = remainder;
    info->half = half;
    info->quarter = quarter;
    return sampleCount;
}
