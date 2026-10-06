/*
 * g3d/g3d_resanm.c - nw4r g3d float and colour animation-channel evaluators (fn_800898B0, fn_80089C6C), the Hermite
 *   step, the colour constructors and the byte-lerp and float-to-short helpers.
 * RANGE. .text 0x800898B0-0x80089F94 (10 functions); extab, extabindex, .data 0x8058FDC8-0x8058FE90 (the evaluators
 *   pass "g3d_resanm.cpp"), .sdata2 0x80795E98-0x80795EC0.  The byte-lerp helpers fn_80089E9C..fn_80089F90 carry no
 *   data reference, so 0x80089E9C is an alternative right edge.
 * NAMES. Map stems.  The retail TU is C++; the source stays `.c` because it is C-idiom and does not compile as C++
 *   without casts, so the `Panic` mangling is spelled verbatim.
 * RESIDUALS. fn_800898B0: the panic strings share a pooled base (`lis r4,...data.0@ha; addi r31`), so ours saves r29
 *   too and takes a 0x50 frame where retail's is 0x40 (0x2D0 of retail's 0x2D8 bytes).
 *   fn_80089C6C: the key-index addressing (`slwi`/`lwzx`) is scheduled and allocated differently.
 *   fn_80089F78: retail truncates float to short with `psq_st f1,0(r3),1,qr5` + `lha`; no source shape or flag
 *   reproduces it.
 *   flipcheck: `.text` 0x6C4 of 0x6E4; `.data` 0xC4 of 0xC8; `.sdata2` 0x24 of 0x28.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_8058FDD8`,
 *     `lbl_8058FDC8`, `lbl_8058FE0C`, `lbl_8058FE50`, `dtor_8005B228`.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma fp_contract off`: retail has no record-form instruction and
 *   no fused multiply-add.
 */


#include "types.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_scnroot.h" /* fn_80082F18 (rule 2) */
#include "g3d/fn_80063888.h" /* fn_800651BC, owned by g3d/fn_80063888.cpp (rule 2) */

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) */
extern void Panic__Q24nw4r2dbFPCciPCce(const char *file, int line, const char *msg, ...);

/* The math/resource helpers this unit calls. */
extern f32 anim_tick_angle(u16 value);
extern f32 math_reciprocal(f32 value);
extern void fn_8005B1B4(u32 *self, u32 value);
extern f32 fn_80463F34(f32 *out, f32 frame);

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
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame);
f32 fn_80089B88(f32 v0, f32 t0, f32 v1, f32 t1, f32 delta, f32 span);
u32 fn_80089C6C(u32 *arg0, f32 frame);
ResAnmClr *fn_80089E9C(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3);
void fn_80089ECC(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3);
u8 fn_80089EF0(s32 from, s32 to, s16 factor);
s16 fn_80089F14(f32 value);
void fn_80089F44(f32 *value, s16 *out);
s16 fn_80089F78(f32 value);
f32 fn_80089F90(f32 *out, f32 frame);


/* Evaluates one float channel at a frame and returns the interpolated value. */
f32 fn_800898B0(ResAnmChrChannel *pData, f32 frame)
{
    ResAnmChrKey *pKey;
    ResAnmChrKey *pLast;
    u16 pos;
    f32 delta;

    u32 valid = IS_VALID_PTR(pData);

    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 105,
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
    pos = fn_80082F18(delta * anim_tick_angle(pData->count) * pData->rate);
    if (pos > pData->count - 1) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 135,
            "estimatePos is out of bounds(%f)\nestimatePos <= %f not satisfied.",
            (f32)pos, (f32)(pData->count - 1));
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
    if (frame == pKey->frame) {
        return pKey->value;
    }
    {
        u32 frameValid = pKey->frame <= frame && frame <= pKey[1].frame;

        if (!frameValid) {
            Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 176,
                "frame is out of bounds(%f)\n%f <= frame <= %f not satisfied.", frame,
                pKey->frame, pKey[1].frame);
        }
    }
    return fn_80089B88(pKey->value, pKey->tangent, pKey[1].value, pKey[1].tangent,
                       frame - pKey->frame, pKey[1].frame - pKey->frame);
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
    u32 sp10;
    u32 spC;
    ResAnmClr sp8;
    f32 value;
    s32 index;
    s16 factor;

    u32 valid = IS_VALID_PTR(arg0);

    if (!valid) {
        Panic__Q24nw4r2dbFPCciPCce("g3d_resanm.cpp", 206,
            "NW4R:Pointer Error\npData(=%p) is not valid pointer.", arg0);
    }
    value = (f32)fn_80089F90(&sp14, frame);
    index = (s32)sp14;
    if (value == 0.0f) {
        return arg0[index];
    }
    fn_8005B1B4(&sp10, arg0[index]);
    fn_8005B1B4(&spC, arg0[index + 1]);
    factor = fn_80089F14(32768.0f * value);
    fn_80089E9C(&sp8,
        fn_80089EF0(((ResAnmClr *)&sp10)->b0, ((ResAnmClr *)&spC)->b0, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b1, ((ResAnmClr *)&spC)->b1, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b2, ((ResAnmClr *)&spC)->b2, factor),
        fn_80089EF0(((ResAnmClr *)&sp10)->b3, ((ResAnmClr *)&spC)->b3, factor));
    return fn_800651BC(&sp8);
}

/* Constructs a 4-byte color from its four byte components. */
ResAnmClr *fn_80089E9C(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3)
{
    fn_80089ECC(self, b0, b1, b2, b3);
    return self;
}

/* Stores a 4-byte color from its four byte components. */
void fn_80089ECC(ResAnmClr *self, u32 b0, u32 b1, u32 b2, u32 b3)
{
    self->b0 = b0;
    self->b1 = b1;
    self->b2 = b2;
    self->b3 = b3;
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
s16 fn_80089F78(f32 value)
{
    return (s16)value;
}

/* Tail call into the frame-index helper. */
f32 fn_80089F90(f32 *out, f32 frame)
{
    return fn_80463F34(out, frame);
}
