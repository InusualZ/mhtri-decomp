/*
 * g3d/g3d_resanm.cpp - nw4r g3d float and colour animation-channel evaluators (fn_800898B0, fn_80089C6C), the Hermite
 *   step, the kept copies of nw4r::ut::Color's four-channel constructor and Set, and the byte-lerp and float-to-short
 *   helpers.
 * RANGE. .text 0x800898B0-0x80089F94 (10 functions); extab, extabindex, .data 0x8058FDC8-0x8058FE90 (the evaluators
 *   pass "g3d_resanm.cpp"), .sdata2 0x80795E98-0x80795EC0.
 * FLAGS. cflags_g3d; file-scope `#pragma peephole off`, `#pragma fp_contract off` and `#pragma dont_inline on`, stated
 *   ahead of the includes so `nw4r/ut_Color.h`'s inline members are compiled under them too.
 * NAMES. Map stems for the evaluators and helpers.  0x80089E9C is Color's four-channel constructor (passes r3-r7 to
 *   0x80089ECC and returns `this`) and 0x80089ECC is Color::Set (each channel narrowed with `clrlwi` before its
 *   `stb`: `int` parameters); the constructor's `int` parameters are a GUESS (nw4r's spelling; its body only
 *   forwards them).
 * RESIDUALS. fn_80089C6C calls Color's `operator u32` under the map stem fn_800651BC (`g3d/g3d_anmscn.cpp` defines
 *   it as a C function), so that relocation name differs.  Our object also carries the weak Color copies earlier
 *   link inputs own (`__dt__Q34nw4r2ut5ColorFv`, `Color(u32)`, `operator=(u32)`, `operator u32`, `ToU32ref`).
 *   fn_80089C6C's extab record carries a cleanup range for the Color temporary (0x1F0, length 0x10: action 0x82 destroying sp+8) that retail's 8-byte
 *   record lacks; `throw()` on `operator u32` or on the destructor does not remove it.
 * SHAPES. Retail has no record-form instruction and no fused multiply-add, and calls every inline member out of line.
 *   fn_80089F78 is the OS fast cast (`psq_st` through GQR5, then `lha`) in inline asm.  fn_800898B0 passes the
 *   estimate bounds to Panic as doubles, compares `pKey->frame == frame` and `frame >= pKey->frame`, and computes
 *   the two Hermite intervals before the call.  fn_80089F90 is modf's argument order (frame first: retail sets f1
 *   before r3), and fn_80089C6C's key index is an `int` (retail keeps the index and recomputes `(index + 1) * 4`; an
 *   `s32` is strength-reduced to one byte offset).  fn_80089C6C returns a Color temporary of the four-channel
 *   constructor, converted with `operator u32` and then destroyed (retail's `__dt__Q34nw4r2ut5ColorFv` call).
 */


#pragma peephole off
#pragma fp_contract off
#pragma dont_inline on

#include "types.h"
#include "OS/OSFastCast.h" /* OS_FASTCAST_S16 (rule 1) */
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_scnroot.h" /* math_f32_to_u16 (rule 2) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic (rule 2) */
#include "nw4r/ut_Color.h" /* nw4r::ut::Color (rule 1) */

#pragma pool_data off

/* The math/resource helpers this unit calls. */
extern "C" {
extern f32 anim_tick_angle(u16 value);
extern f32 math_reciprocal(f32 value);
extern f32 modff(f32 *out, f32 frame);
}

/* A single animation key. */
typedef struct {
    f32 frame;   /* +0x00 */
    f32 value;   /* +0x04 */
    f32 tangent; /* +0x08 */
} ResAnmChrKey; /* size: 0xC */

/* A float channel: a key count, a rate and the key array. */
typedef struct {
    u16 count;           /* +0x00 */
    u16 pad_0x02;        /* +0x02 */
    f32 rate;            /* +0x04 */
    ResAnmChrKey keys[]; /* +0x08 */
} ResAnmChrChannel; /* size: 0x8 */

/* A 4-byte color. */
typedef struct {
    u8 b0; /* +0x00 */
    u8 b1; /* +0x01 */
    u8 b2; /* +0x02 */
    u8 b3; /* +0x03 */
} ResAnmClr; /* size: 0x4 */

/* Forward declarations for the unit's own functions. */
extern "C" {
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame);
f32 fn_80089B88(f32 v0, f32 t0, f32 v1, f32 t1, f32 delta, f32 span);
u32 fn_80089C6C(u32 *arg0, f32 frame);
u8 fn_80089EF0(s32 from, s32 to, s16 factor);
s16 fn_80089F14(f32 value);
void fn_80089F44(f32 *value, s16 *out);
s16 fn_80089F78(f32 value);
f32 fn_80089F90(f32 frame, f32 *out);
}


/* Evaluates one float channel at a frame and returns the interpolated value. */
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame)
{
    ResAnmChrKey *pKey;
    ResAnmChrKey *pLast;
    u16 pos;
    f32 delta;

    u32 valid = IS_VALID_PTR(pData);

    if (!valid) {
        nw4r::db::Panic("g3d_resanm.cpp", 105,
            "NW4R:Pointer Error\npData(=%p) is not valid pointer.", pData);
    }
    pLast = &pData->keys[pData->count - 1];
    if (frame <= pData->keys[0].frame) {
        return pData->keys[0].value;
    }
    if (pLast->frame <= frame) {
        return pLast->value;
    }
    delta = frame - pData->keys[0].frame;
    pos = math_f32_to_u16(delta * anim_tick_angle(pData->count) * pData->rate);
    if (pos > pData->count - 1) {
        nw4r::db::Panic("g3d_resanm.cpp", 135,
            "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.",
            (f64)pos, (f64)(pData->count - 1));
    }
    pKey = &pData->keys[pos];
    if (frame < pKey->frame) {
        do {
            pKey--;
        } while (frame < pKey->frame);
    } else {
        do {
            pKey++;
        } while (pKey->frame <= frame);
        pKey--;
    }
    if (pKey->frame == frame) {
        return pKey->value;
    }
    {
        u32 frameValid = frame >= pKey->frame && frame <= pKey[1].frame;

        if (!frameValid) {
            nw4r::db::Panic("g3d_resanm.cpp", 176,
                "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame,
                pKey->frame, pKey[1].frame);
        }
    }
    {
        f32 t = frame - pKey->frame;
        f32 d = pKey[1].frame - pKey->frame;

        return fn_80089B88(pKey->value, pKey->tangent, pKey[1].value, pKey[1].tangent, t, d);
    }
}

/* Hermite-interpolates between two keys over a span. */
f32 fn_80089B88(f32 v0, f32 t0, f32 v1, f32 t1, f32 delta, f32 span)
{
    f32 h = delta * math_reciprocal(span);
    f32 hm1 = h - 1.0f;

    return v0 + (h * (h * (((2.0f * h) - 3.0f) * (v0 - v1)))) +
           (delta * hm1 * ((hm1 * t0) + (h * t1)));
}

/* Evaluates one color channel at a frame and returns the interpolated color. */
u32 fn_80089C6C(u32 *arg0, f32 frame)
{
    f32 sp14;
    f32 value;
    int index;
    s16 factor;

    u32 valid = IS_VALID_PTR(arg0);

    if (!valid) {
        nw4r::db::Panic("g3d_resanm.cpp", 206,
            "NW4R:Pointer Error\npData(=%p) is not valid pointer.", arg0);
    }
    value = (f32)fn_80089F90(frame, &sp14);
    index = (s32)sp14;
    if (value == 0.0f) {
        return arg0[index];
    }
    nw4r::ut::Color sp10(arg0[index]);
    nw4r::ut::Color spC(arg0[index + 1]);
    factor = fn_80089F14(32768.0f * value);
    return nw4r::ut::Color(
        fn_80089EF0(((ResAnmClr *)&sp10)->b0, ((ResAnmClr *)&spC)->b0, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b1, ((ResAnmClr *)&spC)->b1, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b2, ((ResAnmClr *)&spC)->b2, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b3, ((ResAnmClr *)&spC)->b3, factor));
}

/* Linearly interpolates one byte between two values with a 15-bit factor. */
u8 fn_80089EF0(s32 from, s32 to, s16 factor)
{
    return from + ((((to & 0xFF) - (from & 0xFF)) * factor) >> 15);
}

/* Converts a float to a signed short. */
s16 fn_80089F14(f32 value)
{
    s16 out;

    fn_80089F44(&value, &out);
    return out;
}

/* Converts a float through a pointer and writes the signed short result. */
void fn_80089F44(f32 *value, s16 *out)
{
    *out = fn_80089F78(*value);
}

/* Truncates a float to a signed short. */
s16 fn_80089F78(register f32 value)
{
    f32 buffer;
    register f32 *pBuffer = &buffer;
    register s16 result;

    asm {
        psq_st value, 0(pBuffer), 1, OS_FASTCAST_S16
        lha result, 0(pBuffer)
    }
    return result;
}

/* Tail call into the frame-index helper. */
f32 fn_80089F90(f32 frame, f32 *out)
{
    return modff(out, frame);
}
