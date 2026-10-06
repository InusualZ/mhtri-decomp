/* gx/fn_8009ACE4 - the nw4r::g3d fifo indirect-matrix and current-matrix-reset display-list writers.
 * RANGE. .text 0x8009ACE4-0x8009B140 (2 functions); extab 0x80009A18-0x80009A28, extabindex 0x80022B00-0x80022B18,
 *   .sdata2 0x80795F5C-0x80795F6C (the 1.0f/0.5f/2.0f/1024.0f pool the literals emit).
 * FLAGS. cflags of the gx block in configure.py; `#pragma peephole off` keeps the unfused `clrlwi`/`slwi` pairs.
 * NAMES. GDSetIndTexMtx and GDResetCurrentMtx are GUESSES from the nw4r g3d fifo API: the first packs a 2x3
 *   matrix with a shared power-of-two exponent into three BP 0x06+id words, the second writes the CP 0x30/0x40
 *   matrix-index words and XF 0x1018 with the reset values; both are called only from g3d/g3d_state.cpp.
 * RESIDUALS. none.
 * SHAPES. the grow loop is a `do/while` with an inner `exp >= 46` break (a `while` bound becomes an unrolled
 *   `mtctr` loop); the pool literals are written in first-use order 1.0f, 0.5f, 2.0f, 1024.0f.
 */

#include "types.h"
#include "fn_8004CAD8.h"
#include "gx/fn_8009AA78.h"
#include "g3d/g3d_state.h"

#pragma peephole off

/* Normalizes the record's three column pairs to a shared power-of-two exponent and writes them as
 * three packed BP words: two 11-bit magnitudes plus two exponent bits per word. */
void GDSetIndTexMtx(int base, const f32* v) {
    s8 exp = 0;
    f32 m00 = v[0];
    f32 m01 = v[1];
    f32 m02 = v[2];
    f32 m10 = v[4];
    f32 m11 = v[5];
    f32 m12 = v[6];
    f32 a00 = fn_8005220C(m00);
    f32 a01 = fn_8005220C(m01);
    f32 a02 = fn_8005220C(m02);
    f32 a10 = fn_8005220C(m10);
    f32 a11 = fn_8005220C(m11);
    f32 a12 = fn_8005220C(m12);

    if (a00 >= 1.0f || a01 >= 1.0f || a02 >= 1.0f || a10 >= 1.0f
        || a11 >= 1.0f || a12 >= 1.0f) {
        do {
            if (exp >= 46) {
                break;
            }
            exp += 1;
            m00 *= 0.5f;
            m01 *= 0.5f;
            m02 *= 0.5f;
            m10 *= 0.5f;
            m11 *= 0.5f;
            m12 *= 0.5f;
            a00 *= 0.5f;
            a01 *= 0.5f;
            a02 *= 0.5f;
            a10 *= 0.5f;
            a11 *= 0.5f;
            a12 *= 0.5f;
        } while (a00 >= 1.0f || a01 >= 1.0f || a02 >= 1.0f
                 || a10 >= 1.0f || a11 >= 1.0f || a12 >= 1.0f);
    } else if (a00 < 0.5f && a01 < 0.5f && a02 < 0.5f && a10 < 0.5f
               && a11 < 0.5f && a12 < 0.5f) {
        do {
            exp -= 1;
            m00 *= 2.0f;
            m01 *= 2.0f;
            m02 *= 2.0f;
            m10 *= 2.0f;
            m11 *= 2.0f;
            m12 *= 2.0f;
            a00 *= 2.0f;
            a01 *= 2.0f;
            a02 *= 2.0f;
            a10 *= 2.0f;
            a11 *= 2.0f;
            a12 *= 2.0f;
        } while (a00 < 0.5f && a01 < 0.5f && a02 < 0.5f && a10 < 0.5f
                 && a11 < 0.5f && a12 < 0.5f && exp > -17);
    }

    exp += 17;

    fn_800868A0(((base + 6) << 24) | ((exp & 3) << 22)
                | (((s32)(1024.0f * m00) & 0x7FF)
                   | (((s32)(1024.0f * m10) & 0x7FF) << 11)));
    fn_800868A0(((base + 7) << 24) | (((exp >> 2) & 3) << 22)
                | (((s32)(1024.0f * m01) & 0x7FF)
                   | (((s32)(1024.0f * m11) & 0x7FF) << 11)));
    fn_800868A0(((base + 8) << 24) | (((exp >> 4) & 3) << 22)
                | (((s32)(1024.0f * m02) & 0x7FF)
                   | (((s32)(1024.0f * m12) & 0x7FF) << 11)));
}

/* Emits the fixed CP array-base and XF setup this record type needs, then the two payload words. */
void GDResetCurrentMtx(void) {
    fn_8009AC98(0x30, 0x3CF3CF00);
    fn_8009AC98(0x40, 0x00F3CF3C);
    fn_8009AC44(0x1018, 2);
    fn_8009AB1C(0x3CF3CF00);
    fn_8009AB1C(0x00F3CF3C);
}
