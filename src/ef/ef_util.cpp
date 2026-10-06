/*
 * ef/ef_util.cpp - the NW4R effect library's shared math/utility file: the `nw4r::ut::List` to array copy, the
 *   VEC3/MTX34 helpers (rotation basis, sin/cos pairs, column length, scale/axis matrix builders) and the pointer
 *   assert the `ef` band uses.
 * RANGE. .text 0x8009B374-0x8009CDBC (17 functions); extab 0x80009A38-0x80009A98, extabindex 0x80022B30-0x80022BC0,
 *   .data 0x80591948-0x80591C68 (the `__FILE__` string "ef_util.cpp" first), .sdata 0x807912D8-0x807912E0,
 *   .sdata2 0x80795F70-0x80795FB8.  Left edge: `g3d/g3d_gpu.cpp`'s bodies cite "g3d_gpu.cpp" (0x80591900) and pool
 *   their own 0.0f (0x80795F6C) apart from this TU's (0x80795F7C).  Right edge: `fn_8009CDBC` cites
 *   "ef_animcurve.cpp"; the tail `ef_mtx34_column_length` cites none of this TU's strings or pool, only the `.sdata` pair
 *   {3.0f, 0.5f} at 0x807912D8.
 * FLAGS. `cflags_main`; `#pragma peephole off` and `#pragma fp_contract off` around `fn_8009B374`/`fn_8009B448` only
 *   (retail's unfused `clrlwi` + `slwi`, `li r0,<slot>; psq_lx` and `fmuls` + `fsubs` there; file-wide, the pass
 *   costs `fn_8009C6F0`/`ef_sin_cos`).
 * NAMES. The map has only `fn_` stems for the range.
 *   GUESS (from the body and its callers): `ef_sin_cos`, `ef_vec3_normalize_to`.
 *   GUESS: `ef_mtx34_rotate_xyz` (the Euler rotation its callers build), `ef_mtx34_scale_columns` (the
 *   column scale).
 * RESIDUALS. 3 rows unwritten: 0x8009C7D4-0x8009CBA0 (two empty bodies whose whole retail body is the
 *   paired-single sequence, playbook 85) and 0x8009CD64-0x8009CDBC (`ef_mtx34_column_length`, declared, never defined).  The
 *   source order differs from retail's, so `.text`, extab and extabindex run in another order.
 *   2 rows written and at 0: `fn_8009CBA0`, `ef_mtx34_scale_columns` (paired-single bodies, playbook 85).
 *   7 partial rows:
 *  - `ef_vec3_normalize_to`, `fn_8009C6F0`, `ef_sin_cos`: paired-single bodies (playbook 85); our C versions also read
 *    `lbl_80795FA8`/`lbl_80795F7C` where retail does not;
 *  - `fn_8009B448`: the float registers of the assert's distance test differ;
 *  - `fn_8009BCB4`: the FPR restores are `psq_l <off>(r1)` where retail has `li r0,<off>; psq_lx` (the
 *    peephole region does not cover it);
 *  - `fn_8009C040`: the frame is 0x50 against retail's 0x60;
 *  - `fn_8009CCAC`: the two clamp branches store the loaded constant from f0 where retail uses f1.
 *   relocdiff: `fn_8009B840` and `fn_8009BA78` score 100 but retail passes per-function copies of the
 *   "ef_util.cpp" string (`lbl_80591B98`/`lbl_80591BD8`, `lbl_80591C18`/`lbl_80591C58`) where ours passes
 *   `lbl_80591948`.
 *   flipcheck: `.data` and `.sdata` claimed, not emitted; `.sdata2` 0x8 of 0x48; `.text` 0x16B8 of 0x1A48;
 *   extab 0x50 of 0x60; extabindex 0x78 of 0x90.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_80501C60`,
 *     `List_GetNext__Q24nw4r2utFPCQ34nw4r2ut4ListPCv`, `lbl_80591948`, `lbl_80591BD8`, `lbl_80591B98`,
 *     `lbl_80591C58`, `lbl_80591C18`, `lbl_80795F7C`, `lbl_80795F84`, `fn_8050133C`,
 *     `MTX34Scale__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_8050133C`, `fn_80501C60`.
 *   `ef_mtx34_rotate_xyz` (0x8009CA30) is unwritten (an empty body; 60 paired-single instructions).
 * SHAPES. The pointer assert is the six-BOOL chain with a two-test first `if` (`ef/ef_point.cpp`'s shape); its
 *   trailing `(ptr)` is retail's fourth `Panic` argument (`mr r6, <ptr>`).
 */

#include "types.h"
#include "fn_8004CAD8.h"    /* the VEC3 helpers this range owns (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor - owner src/mh3_pad.cpp (rule 2) */
#include "g3d/g3d_anmchr.h" /* fn_800610AC - owner src/g3d/g3d_anmchr.cpp (rule 2) */
#include "nw4r/math_triangular.h" /* nw4r::math::sSinCosTbl - owner src/nw4r/math_triangular.cpp (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db

namespace math {

/* `FrSqrt__Q24nw4r4mathFf`, the reciprocal square root the scale path uses. */
f32 FrSqrt(f32 x);

} // namespace math
} // namespace nw4r

/* Helpers declared locally: the `ut::List` iterator, this unit's own unwritten tail and three math
 * callees. */
extern "C" {
void* fn_80501C60(void* list, void* node);
/* 0x8009CD64 - the matrix-axis scale helper fn_8009BCB4 reaches (unwritten). */
f32 ef_mtx34_column_length(const f32* mtx, s32 index);
f32 fn_80463DE4(f32 x);
f32 atan2f(f32 y, f32 x);
f32 fn_8005A63C(f32 x);

/* This unit's own bodies, in address order: a forward declaration for the ones a later body calls
 * (the owner is this file, so rule 2 is satisfied by construction). */
u16 fn_8009B374(void* list, void** buf, s32 size);
void fn_8009B448(f32* mtx, const f32* vec);
void fn_8009B650(const f32* src, f32* mtx);
void fn_8009B840(f32* mtx, s32 index, const f32* vec);
f32* fn_8009BA78(const f32* mtx, s32 index, f32* vec);
void fn_8009BCB4(const f32* vec, f32* mtx);
void fn_8009BF08(const f32* mtx, f32* vec);
s32 ef_vec3_normalize_to(f32* dst, const f32* src);
void fn_8009C7D4(f32* mtx, const f32* a, const f32* b);
void ef_mtx34_rotate_xyz(f32* mtx, f32 x, f32 y, f32 z);
void fn_8009CCAC(f32* dst, const f32* mtx, const f32* scale);
void fn_8050133C(f32* dst, const f32* mtx, const f32* scale);
}

/* This unit's strings and pooled floats (its claimed `.data`/`.sdata2`) and the sin/cos table, declared,
 * never defined. */
extern char lbl_80591948[]; /* "ef_util.cpp"                                                       .data */
extern char lbl_80591954[]; /* "NW4R:Failed assertion list != NULL"                               .data */
extern char lbl_80591978[]; /* "NW4R:Failed assertion array != NULL"                              .data */
extern char lbl_8059199C[]; /* "NW4R:Pointer Error\nmtx(=%p) is not valid pointer."               .data */
extern char lbl_805919D0[]; /* "NW4R:Failed assertion ... FAbs(VEC3Len(&vec) - 1.0F) < epsilon"   .data */
extern char lbl_80591A24[]; /* "NW4R:Pointer Error\ndst(=%p) is not valid pointer."               .data */
extern char lbl_80591A58[]; /* "NW4R:Pointer Error\nrotate(=%p) is not valid pointer."            .data */
extern char lbl_80591A90[]; /* "NW4R:Pointer Error\ntranslate(=%p) is not valid pointer."         .data */
extern char lbl_80591AC8[]; /* "NW4R:Pointer Error\nscale(=%p) is not valid pointer."             .data */
extern char lbl_80591AFC[]; /* "NW4R:Pointer Error\nvec(=%p) is not valid pointer."               .data */
extern char lbl_80591B30[]; /* "NW4R:Pointer Error\npOut(=%p) is not valid pointer."              .data */
extern char lbl_80591B64[]; /* "NW4R:Pointer Error\nvec(=%p) is not valid pointer."               .data */
extern char lbl_80591B98[]; /* "ef_util.cpp"                                                      .data */
extern char lbl_80591BA4[]; /* "NW4R:Pointer Error\nmtx(=%p) is not valid pointer."               .data */
extern char lbl_80591BD8[]; /* "ef_util.cpp"                                                      .data */
extern char lbl_80591BE4[]; /* "NW4R:Pointer Error\nvec(=%p) is not valid pointer."               .data */
extern char lbl_80591C18[]; /* "ef_util.cpp"                                                      .data */
extern char lbl_80591C24[]; /* "NW4R:Pointer Error\nmtx(=%p) is not valid pointer."               .data */
extern char lbl_80591C58[]; /* "ef_util.cpp"                                                      .data */

extern const f32 lbl_80795F70[]; /* {65536.0f, 65536.0f} - the paired-single 65536 constant   .sdata2 */
extern const f32 lbl_80795F78;   /* 40.743664f = 256 / (2 * pi) - radians to 8-bit fidx       .sdata2 */
extern const f32 lbl_80795F7C;   /* 0.0f                                                     .sdata2 */
extern const f32 lbl_80795F80;   /* 1.0f                                                     .sdata2 */
extern const f32 lbl_80795F84;   /* 1.0e-05f                                                 .sdata2 */
extern const f32 lbl_80795F88;   /* -1.0f                                                    .sdata2 */
extern const f32 lbl_80795F8C;   /* 1.19209290e-07f (FLT_EPSILON)                            .sdata2 */
extern const f32 lbl_80795F90;   /* 1.17549435e-38f (FLT_MIN)                                .sdata2 */
extern const f64 lbl_80795F98;   /* -1.0 (loaded with lfd)                                   .sdata2 */
extern const f32 lbl_80795FA0;   /* 0.5f                                                     .sdata2 */
extern const f32 lbl_80795FA4;   /* 3.0f                                                     .sdata2 */
extern const f32 lbl_80795FA8;   /* 65536.0f - the quantized angle's ceiling                  .sdata2 */
extern const f32 lbl_80795FAC;   /* 100000.0f                                                .sdata2 */
extern const f32 lbl_80795FB0[]; /* {1.0e-05f, ...} - address taken                           .sdata2 */

/* The library's pointer assert: `ptr` must fall in one of the seven mapped memory ranges (six
 * materialised BOOLs, the first `if` carrying two tests). */
#define NW4R_POINTER_ASSERT(ptr, line, msg)                                                        \
    {                                                                                              \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;         \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                       \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))                \
            ok6_ = FALSE;                                                                          \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                                  \
            ok5_ = FALSE;                                                                          \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                       \
            ok4_ = FALSE;                                                                          \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                                  \
            ok3_ = FALSE;                                                                          \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                                  \
            ok2_ = FALSE;                                                                          \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                                  \
            ok1_ = FALSE;                                                                          \
        if (!ok1_)                                                                                 \
            nw4r::db::Panic(lbl_80591948, line, msg, (ptr));                                       \
    }

/* The library's condition assert: the message is the expression's own text, and the call passes no
 * pointer (the target's three-argument `Panic`). */
#define NW4R_ASSERT(cond, line, msg)                                                               \
    {                                                                                              \
        if (!(cond))                                                                               \
            nw4r::db::Panic(lbl_80591948, line, msg);                                              \
    }

/* --------------------------------------------------------------------------------------------- */
/* The bodies, in address order.                                                                  */
/* --------------------------------------------------------------------------------------------- */

/* The two bodies below keep retail's unfused forms (playbook 39); the reset follows them. */
#pragma peephole off
#pragma fp_contract off

/* Copies a `nw4r::ut::List`'s objects into `buf`, stopping at `size` entries, and returns how many were
 * written.  The list iterator returns NULL at the end of the list. */
extern "C" u16 fn_8009B374(void* list, void** buf, s32 size) {
    u16 count;
    void* node;

    NW4R_ASSERT(list != NULL, 0x25, lbl_80591954);
    NW4R_ASSERT(buf != NULL, 0x26, lbl_80591978);

    count = 0;
    node = NULL;
    while ((node = fn_80501C60(list, node)) != NULL) {
        u16 idx = count++;

        buf[idx] = node;
        if (count >= size)
            break;
    }
    return count;
}

/* Builds the orthonormal basis whose third row is the unit vector `vec`, into the 3x4 matrix `mtx`. */
extern "C" void fn_8009B448(f32* mtx, const f32* vec) {
    f32 s;
    f32 a;
    f32 b;
    f32 z;

    NW4R_POINTER_ASSERT(mtx, 0x1AE, lbl_8059199C);
    NW4R_ASSERT(abs_f32(fn_80050F24(vec) - lbl_80795F80) < lbl_80795F84, 0x1AF, lbl_805919D0);

    z = vec[2];
    s = abs_f32(z);
    if (lbl_80795F80 - s < lbl_80795F8C) {
        a = lbl_80795F7C;
        b = lbl_80795F7C;
    } else {
        s = sqrt_f32(lbl_80795F80 - z * z);
        a = vec[1] / s;
        b = vec[0] / -s;
    }
    mtx[0] = a;
    mtx[1] = vec[0];
    mtx[2] = z * b;
    mtx[3] = lbl_80795F7C;
    mtx[4] = b;
    mtx[5] = vec[1];
    mtx[6] = -a * z;
    mtx[7] = lbl_80795F7C;
    mtx[8] = lbl_80795F7C;
    mtx[9] = vec[2];
    mtx[10] = s;
    mtx[11] = lbl_80795F7C;
}

#pragma peephole on
#pragma fp_contract on

/* Builds an orthonormal 3x4 matrix from the first two columns of `src` (Gram-Schmidt): the first column
 * is normalised, the second is made perpendicular to it, and the third is their cross product. */
extern "C" void fn_8009B650(const f32* src, f32* mtx) {
    f32 v20[3];
    f32 v14[3];
    f32 v8[3];

    NW4R_POINTER_ASSERT(mtx, 0x1F6, lbl_80591A24);

    VEC3_ctor((nw4r::math::VEC3*)v20);
    VEC3_ctor((nw4r::math::VEC3*)v14);
    VEC3_ctor((nw4r::math::VEC3*)v8);

    fn_8009BA78(src, 0, v20);
    if (ef_vec3_normalize_to(v20, v20) == 0)
        v20[0] = lbl_80795F80;

    fn_8009BA78(src, 1, v14);
    if (ef_vec3_normalize_to(v14, v14) == 0)
        v14[1] = lbl_80795F80;

    vec3_cross(v8, v20, v14);
    vec3_cross(v14, v8, v20);

    fn_8009B840(mtx, 0, v20);
    fn_8009B840(mtx, 1, v14);
    fn_8009B840(mtx, 2, v8);
    mtx[3] = lbl_80795F7C;
    mtx[7] = lbl_80795F7C;
    mtx[11] = lbl_80795F7C;
}

/* Writes `vec` into column `index` of the 3x4 matrix `mtx`. */
extern "C" void fn_8009B840(f32* mtx, s32 index, const f32* vec) {
    NW4R_POINTER_ASSERT(mtx, 0x1E5, lbl_80591BA4);
    NW4R_POINTER_ASSERT(vec, 0x1E6, lbl_80591B64);

    mtx[index] = vec[0];
    mtx[index + 4] = vec[1];
    mtx[index + 8] = vec[2];
}

/* Reads column `index` of the 3x4 matrix `mtx` into `vec`, and returns `vec`. */
extern "C" f32* fn_8009BA78(const f32* mtx, s32 index, f32* vec) {
    NW4R_POINTER_ASSERT(mtx, 0x1D8, lbl_80591C24);
    NW4R_POINTER_ASSERT(vec, 0x1D9, lbl_80591BE4);

    vec[0] = mtx[index];
    vec[1] = mtx[index + 4];
    vec[2] = mtx[index + 8];
    return vec;
}

/* Decomposes the rotation part of the 3x4 matrix `mtx` into the Euler angles at `rot`, zeroing them when
 * any of the three columns collapses (a length below FLT_MIN). */
extern "C" void fn_8009BCB4(const f32* mtx, f32* rot) {
    f32 sx;
    f32 sy;
    f32 sz;
    f32 t;

    NW4R_POINTER_ASSERT(rot, 0x21B, lbl_80591A58);

    for (;;) {
        sx = ef_mtx34_column_length(mtx, 0);
        if (sx < lbl_80795F90)
            break;
        sy = ef_mtx34_column_length(mtx, 1);
        if (sy < lbl_80795F90)
            break;
        sz = ef_mtx34_column_length(mtx, 2);
        if (sz < lbl_80795F90)
            break;

        t = -mtx[8] / sx;
        if (t > lbl_80795F80)
            t = lbl_80795F80;
        if (t < lbl_80795F88)
            t = lbl_80795F88;
        rot[1] = fn_80463DE4(t);
        if (fn_8005A63C(rot[1]) >= lbl_80795F90) {
            rot[0] = atan2f(mtx[9] / sy, mtx[10] / sz);
            rot[2] = atan2f(mtx[4], mtx[0]);
        } else {
            rot[0] = atan2f(mtx[1], mtx[5]);
            rot[2] = lbl_80795F7C;
        }
        return;
    }
    rot[0] = lbl_80795F7C;
    rot[1] = lbl_80795F7C;
    rot[2] = lbl_80795F7C;
}

/* Reads the translation column of the 3x4 matrix `mtx` into `vec`. */
extern "C" void fn_8009BF08(const f32* mtx, f32* vec) {
    NW4R_POINTER_ASSERT(vec, 0x259, lbl_80591A90);

    vec[0] = mtx[3];
    vec[1] = mtx[7];
    vec[2] = mtx[11];
}

/* Writes the sine and cosine of `angle` as a two-float pair at `pOut`. */
extern "C" void fn_8009C6F0(f32* pOut, f32 angle) {
    f32 y = angle * lbl_80795F78;
    f32 a = __fabs(y);
    f32 s;
    f32 c;
    const f32* tbl;

    pOut[0] = a;
    if (a > lbl_80795FA8) {
        do {
            a -= lbl_80795FA8;
        } while (a >= lbl_80795FA8);
        pOut[0] = a;
    }
    tbl = &nw4r::math::sSinCosTbl[0].sin_val + (*(u16*)pOut & 0xFF) * 4;
    a = a - pOut[0];
    s = tbl[0] + tbl[2] * a;
    c = tbl[1] + tbl[3] * a;
    if (y < lbl_80795F7C)
        s = -s;
    pOut[0] = s;
    pOut[1] = c;
}

/* Writes the sine of `angle` to `pSin` and its cosine to `pCos`. */
extern "C" void ef_sin_cos(f32* pSin, f32* pCos, f32 angle) {
    f32 y = angle * lbl_80795F78;
    f32 a = __fabs(y);
    f32 s;
    f32 c;
    const f32* tbl;

    pSin[0] = a;
    if (a > lbl_80795FA8) {
        do {
            a -= lbl_80795FA8;
        } while (a >= lbl_80795FA8);
        pSin[0] = a;
    }
    tbl = &nw4r::math::sSinCosTbl[0].sin_val + (*(u16*)pSin & 0xFF) * 4;
    a = a - pSin[0];
    s = tbl[0] + tbl[2] * a;
    c = tbl[1] + tbl[3] * a;
    pCos[0] = c;
    if (y < lbl_80795F7C)
        s = -s;
    pSin[0] = s;
}

/* Builds a rotation matrix from three Euler angles: a pointer assert, then 46 paired-single instructions
 * sharing fn_8009C6F0's table lookup and reduce-to-65536 loops (unwritten). */
extern "C" void fn_8009C7D4(f32* mtx, const f32* a, const f32* b) {
}

/* Builds a rotation matrix from three Euler angles: 60 paired-single instructions and nothing else
 * (unwritten). */
extern "C" void ef_mtx34_rotate_xyz(f32* mtx, f32 x, f32 y, f32 z) {
}

/* Computes the 3x4 matrix's three column scales into `scale` (Gram-Schmidt lengths, negated for a
 * left-handed basis; a collapsed column gives 0). */
extern "C" void fn_8009C040(const f32* mtx, f32* scale) {
    f32 v8[3];
    f32 v14[3];
    f32 v20[3];
    f32 v2c[3];
    f32 d0;
    f32 d1;
    f32 d2;
    f32 len;
    f32 r;

    NW4R_POINTER_ASSERT(scale, 0x266, lbl_80591AC8);

    VEC3_ctor((nw4r::math::VEC3*)v8);
    VEC3_ctor((nw4r::math::VEC3*)v14);
    VEC3_ctor((nw4r::math::VEC3*)v20);
    VEC3_ctor((nw4r::math::VEC3*)v2c);

    fn_8009BA78(mtx, 0, v2c);
    len = vec3_length_sq(v2c);
    if (len > lbl_80795F8C) {
        r = nw4r::math::FrSqrt(len);
        scale[0] = math_reciprocal(r);
        vec3_scale_by(v2c, v2c, r);

        fn_8009BA78(mtx, 1, v20);
        d0 = vec3_dot(v2c, v20);
        vec3_scale_by(v8, v2c, d0);
        PSVECSubtract(v20, v20, v8);

        len = vec3_length_sq(v20);
        if (len > lbl_80795F8C) {
            r = nw4r::math::FrSqrt(len);
            scale[1] = math_reciprocal(r);
            d0 = d0 * r;
            vec3_scale_by(v20, v20, r);

            fn_8009BA78(mtx, 2, v14);
            d1 = vec3_dot(v20, v14);
            vec3_scale_by(v8, v20, d1);
            PSVECSubtract(v14, v14, v8);
            d2 = vec3_dot(v2c, v14);
            vec3_scale_by(v8, v2c, d2);
            PSVECSubtract(v14, v14, v8);

            len = vec3_length_sq(v14);
            if (len > lbl_80795F8C) {
                scale[2] = sqrt_f32(len);
                vec3_cross(v8, v20, v14);
                if (vec3_dot(v2c, v8) < lbl_80795F7C) {
                    scale[0] = scale[0] * -1.0;
                    scale[1] = scale[1] * -1.0;
                    scale[2] = scale[2] * -1.0;
                }
            } else {
                scale[2] = lbl_80795F7C;
            }
        } else {
            scale[1] = lbl_80795F7C;
            fn_8009BA78(mtx, 2, v14);
            d2 = vec3_dot(v2c, v14);
            vec3_scale_by(v8, v2c, d2);
            PSVECSubtract(v14, v14, v8);
            len = vec3_length_sq(v14);
            if (len > lbl_80795F8C)
                scale[2] = sqrt_f32(len);
            else
                scale[2] = lbl_80795F7C;
        }
    } else {
        scale[0] = lbl_80795F7C;
        fn_8009BA78(mtx, 1, v20);
        len = vec3_length_sq(v20);
        if (len > lbl_80795F8C) {
            r = nw4r::math::FrSqrt(len);
            scale[1] = math_reciprocal(r);
            vec3_scale_by(v20, v20, r);

            fn_8009BA78(mtx, 2, v14);
            d1 = vec3_dot(v20, v14);
            vec3_scale_by(v8, v20, d1);
            PSVECSubtract(v14, v14, v8);
            scale[2] = fn_80050F24(v14);
        } else {
            scale[1] = lbl_80795F7C;
            fn_8009BA78(mtx, 2, v14);
            scale[2] = fn_80050F24(v14);
        }
    }
}

/* Normalises `vec` into `dst` and returns 1, or copies `vec` unchanged and returns 0 when it is the zero
 * vector. */
extern "C" s32 ef_vec3_normalize_to(f32* dst, const f32* vec) {
    f32 x;
    f32 y;
    f32 z;
    f32 len;
    f32 r;
    f32 s;

    NW4R_POINTER_ASSERT(dst, 0x2DB, lbl_80591A24);
    NW4R_POINTER_ASSERT(vec, 0x2DC, lbl_80591AFC);

    x = vec[0];
    y = vec[1];
    z = vec[2];
    len = x * x + y * y + z * z;
    if (len == lbl_80795F7C) {
        dst[0] = x;
        dst[1] = y;
        dst[2] = z;
        return 0;
    }
    r = __frsqrte(len);
    s = (lbl_80795FA4 - len * r * r) * (lbl_80795FA0 * r);
    dst[0] = x * s;
    dst[1] = y * s;
    dst[2] = z * s;
    return 1;
}

/* Scales every column of the 3x4 matrix `mtx` by the matching component of `scale`, into `dst`; a zero
 * component is replaced by 1.0e-05f. */
extern "C" void fn_8009CBA0(f32* dst, const f32* mtx, const f32* scale) {
    f32 sx = scale[0];
    f32 sy = scale[1];
    f32 sz = scale[2];

    if (sx == lbl_80795F7C)
        sx = lbl_80795F84;
    if (sy == lbl_80795F7C)
        sy = lbl_80795F84;
    if (sz == lbl_80795F7C)
        sz = lbl_80795F84;

    dst[0] = mtx[0] * sx;
    dst[1] = mtx[1] * sy;
    dst[2] = mtx[2] * sz;
    dst[3] = mtx[3];
    dst[4] = mtx[4] * sx;
    dst[5] = mtx[5] * sy;
    dst[6] = mtx[6] * sz;
    dst[7] = mtx[7];
    dst[8] = mtx[8] * sx;
    dst[9] = mtx[9] * sy;
    dst[10] = mtx[10] * sz;
    dst[11] = mtx[11];
}

/* Scales every column of the 3x4 matrix `mtx` by the matching component of `scale`, into `dst`; a zero
 * component is replaced by the pooled ramp value at lbl_80795FB0. */
extern "C" void ef_mtx34_scale_columns(f32* dst, const f32* scale, const f32* mtx) {
    f32 sx = scale[0];
    f32 sy = scale[1];
    f32 sz = scale[2];
    f32 d = lbl_80795FB0[0];

    if (sx == lbl_80795F7C)
        sx = d;
    if (sy == lbl_80795F7C)
        sy = d;
    if (sz == lbl_80795F7C)
        sz = d;

    dst[0] = mtx[0] * sx;
    dst[1] = mtx[1] * sx;
    dst[2] = mtx[2] * sx;
    dst[3] = mtx[3] * sx;
    dst[4] = mtx[4] * sy;
    dst[5] = mtx[5] * sy;
    dst[6] = mtx[6] * sy;
    dst[7] = mtx[7] * sy;
    dst[8] = mtx[8] * sz;
    dst[9] = mtx[9] * sz;
    dst[10] = mtx[10] * sz;
    dst[11] = mtx[11] * sz;
}

/* Builds the reciprocal of the scale vector `scale` (a zero component becomes 100000.0f) and applies it
 * to the 3x4 matrix `mtx` through fn_8050133C. */
extern "C" void fn_8009CCAC(f32* dst, const f32* mtx, const f32* scale) {
    f32 inv[3];

    VEC3_ctor((nw4r::math::VEC3*)inv);
    if (lbl_80795F7C != scale[0])
        inv[0] = math_reciprocal(scale[0]);
    else
        inv[0] = lbl_80795FAC;
    if (lbl_80795F7C != scale[1])
        inv[1] = math_reciprocal(scale[1]);
    else
        inv[1] = lbl_80795FAC;
    if (lbl_80795F7C != scale[2])
        inv[2] = math_reciprocal(scale[2]);
    else
        inv[2] = lbl_80795FAC;
    fn_8050133C(dst, mtx, inv);
}
