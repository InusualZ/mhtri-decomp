/* WENC/wenc.cpp - the Wii remote speaker encoder (`WENCGetEncodeData`): 16-bit samples to 4-bit adaptive differential codes.
 * RANGE. .text 0x804EB22C-0x804EB510 (1 function); .rodata 0x80573C40-0x80573C80; .sdata2 0x8079D3C0-0x8079D3C8.
 *   Edges: its .rodata table and .sdata2 literal are read only by this function; both edges are 4-aligned (the
 *   function starts at 0x804EB22C, so the TU compiled without 16-byte function alignment, unlike its neighbours,
 *   whose starts are all 16-aligned).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`).
 * NAMES. `WENCGetEncodeData` is the map's name; the state record `WENCInfo` and its field names are read from the algorithm.
 * RESIDUALS. The unit's data is not claimed: the step scale table (.rodata 0x80573C40, copied to the stack by the body) and the
 *   int-to-float constant (.sdata2 0x8079D3C0) are emitted by this source.
 */

#include "types.h"
#include "WENC/wenc.h"
#include "Runtime.PPCEABI.H/memset.h"

/* Encodes `sampleCount` samples into `out`, high nibble first; returns `sampleCount`. */
s32 WENCGetEncodeData(WENCInfo* info, u32 flags, const s16* samples, s32 sampleCount, u8* out)
{
    const f64 stepScale[8] = { 0.8984375, 0.8984375, 0.8984375, 0.8984375, 1.19921875, 1.59765625, 2.0, 2.3984375 };
    s32 predictor;
    s32 step;
    s32 delta;
    s32 remainder;
    s32 half;
    s32 quarter;
    s32 i;

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
        s16 sample = samples[i];
        s32 sign = 0;
        s32 bit2 = 0;
        s32 bit1 = 0;
        s32 bit0 = 0;
        s32 diff;
        s32 code;

        if (sample < predictor) {
            sign = 1;
        }
        diff = sample - predictor;
        remainder = (diff < 0) ? -diff : diff;
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
        delta = (1 - sign * 2) * (quarter * bit0 + step * bit2 + (quarter / 2 + half * bit1));
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
        code = bit2 * 4 + bit1 * 2 + bit0;
        out[i / 2] |= (u8)(sign * 8 + code) << (((i & 1) - 1) & 4);
        step = (s32)((f64)step * stepScale[code]);
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
