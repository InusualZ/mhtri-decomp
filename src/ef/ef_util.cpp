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
 * FLAGS. `cflags_main`; `#pragma peephole off` and `#pragma fp_contract off` around `fn_8009B374`/`ef_mtx34_from_y_axis` only
 *   (retail's unfused `clrlwi` + `slwi`, `li r0,<slot>; psq_lx` and `fmuls` + `fsubs` there; file-wide, the pass
 *   costs `ef_vec_sin_cos`/`ef_sin_cos`).
 * NAMES. The map has only `fn_` stems for the range.
 *   GUESS: `ef_vec_sin_cos` (0x8009C6F0): writes an angle's sine and cosine into a vector's x and y.
 *   GUESS: `ef_mtx34_from_y_axis` (0x8009B448): builds an orthonormal frame whose Y axis is a unit direction.
 *   GUESS: `ef_vec3_from_rotation` (0x8009C7D4): turns three rotation angles into the direction the particle
 *   manager's gravity and spin fields use.
 *   GUESS (from the body and its callers): `ef_sin_cos`, `ef_vec3_normalize_to`.
 *   GUESS: `ef_mtx34_rotate_xyz` (the Euler rotation its callers build), `ef_mtx34_scale_columns` (the
 *   column scale).
 *   GUESS: `ef_mtx34_column_length` (0x8009CD64): the length of one column of a 3x4 matrix.
 *   GUESS (from their values and use): `ef_util_f32_65536_pair`, `ef_util_f32_rad_to_fidx`, `ef_util_f32_zero`,
 *   `ef_util_f32_one`, `ef_util_f32_min_scale`, `ef_util_f32_minus_one`, `ef_util_f32_epsilon`, `ef_util_f32_flt_min`,
 *   `ef_util_f64_minus_one`, `ef_util_f32_half`, `ef_util_f32_three`, `ef_util_f32_65536`, `ef_util_f32_100000`,
 *   `ef_util_f32_min_scale_pair`, `ef_util_rsqrt_three_half`.
 * RESIDUALS. The source order differs from retail's, so `.text`, extab and extabindex run in another order.
 *  - `ef_mtx34_from_y_axis`: the float registers of the assert's distance test differ;
 *  - `fn_8009BCB4`: the FPR restores are `psq_l <off>(r1)` where retail has `li r0,<off>; psq_lx` (the
 *    peephole region does not cover it);
 *  - `fn_8009C040`: the frame is 0x50 against retail's 0x60;
 *  - `fn_8009CCAC`: the two clamp branches store the loaded constant from f0 where retail uses f1.
 *   relocdiff: `fn_8009B840` and `fn_8009BA78` score 100 but retail passes per-function copies of the
 *   "ef_util.cpp" string (`lbl_80591B98`/`lbl_80591BD8`, `lbl_80591C18`/`lbl_80591C58`) where ours passes
 *   `lbl_80591948`.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x8 of 0x48 (the pool is declared); `.text` 0x1A0C of
 *   0x1A48; extab and extabindex differ in the open rows' records.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_80501C60`,
 *     `List_GetNext__Q24nw4r2utFPCQ34nw4r2ut4ListPCv`, `lbl_80591948`, `lbl_80591BD8`, `lbl_80591B98`,
 *     `lbl_80591C58`, `lbl_80591C18`, `ef_util_f32_zero`, `ef_util_f32_min_scale`, `ef_util_f64_minus_one`, `fn_8050133C`,
 *     `MTX34Scale__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_8050133C`, `fn_80501C60`.
 * SHAPES. The paired-single rows are retail's own assembly (playbook 85: no compiler emits them): `ef_vec_sin_cos`,
 *   `ef_sin_cos`, `fn_8009CBA0`, `ef_mtx34_scale_columns` and `ef_mtx34_column_length` are `asm` functions
 *   (`nofralloc`, no extab record, as in retail); `ef_mtx34_rotate_xyz`, `ef_vec3_from_rotation` and
 *   `ef_vec3_normalize_to` are C functions around an `asm` block (retail gives them a frame and an extab record).
 *   Inside a C function every physical register an `asm` block names is reserved for the whole function, so
 *   those blocks name their GPRs through `register` locals and the asserts before them keep retail's registers.
 * SHAPES. The pointer assert is the six-BOOL chain with a two-test first `if` (`ef/ef_point.cpp`'s shape); its
 *   trailing `(ptr)` is retail's fourth `Panic` argument (`mr r6, <ptr>`).
 */

#include "types.h"
#include "fn_8004CAD8.h"    /* the VEC3 helpers this range owns (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor - owner src/mh3_pad.cpp (rule 2) */
#include "g3d/g3d_anmchr.h" /* fn_800610AC - owner src/g3d/g3d_anmchr.cpp (rule 2) */
#include "nw4r/math_triangular.h" /* nw4r::math::sSinCosTbl - owner src/nw4r/math_triangular.cpp (rule 2) */
#include "ef/ef_util.h" /* the unit's own header */

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
f32 fn_80463DE4(f32 x);
f32 atan2f(f32 y, f32 x);
f32 fn_8005A63C(f32 x);

/* This unit's own bodies, in address order: a forward declaration for the ones a later body calls
 * (the owner is this file, so rule 2 is satisfied by construction). */
u16 fn_8009B374(void* list, void** buf, s32 size);

void fn_8009B650(const f32* src, f32* mtx);
void fn_8009B840(f32* mtx, s32 index, const f32* vec);
f32* fn_8009BA78(const f32* mtx, s32 index, f32* vec);
void fn_8009BCB4(const f32* vec, f32* mtx);
void fn_8009BF08(const f32* mtx, f32* vec);
s32 ef_vec3_normalize_to(f32* dst, const f32* src);
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

extern const f32 ef_util_f32_65536_pair[2]; /* {65536.0f, 65536.0f} - the paired-single 65536 constant   .sdata2 */
extern const f32 ef_util_f32_rad_to_fidx;   /* 40.743664f = 256 / (2 * pi) - radians to 8-bit fidx       .sdata2 */
extern const f32 ef_util_f32_zero;   /* 0.0f                                                     .sdata2 */
extern const f32 ef_util_f32_one;   /* 1.0f                                                     .sdata2 */
extern const f32 ef_util_f32_min_scale;   /* 1.0e-05f                                                 .sdata2 */
extern const f32 ef_util_f32_minus_one;   /* -1.0f                                                    .sdata2 */
extern const f32 ef_util_f32_epsilon;   /* 1.19209290e-07f (FLT_EPSILON)                            .sdata2 */
extern const f32 ef_util_f32_flt_min;   /* 1.17549435e-38f (FLT_MIN)                                .sdata2 */
extern const f64 ef_util_f64_minus_one;   /* -1.0 (loaded with lfd)                                   .sdata2 */
extern const f32 ef_util_f32_half;   /* 0.5f                                                     .sdata2 */
extern const f32 ef_util_f32_three;   /* 3.0f                                                     .sdata2 */
extern const f32 ef_util_f32_65536;   /* 65536.0f - the quantized angle's ceiling                  .sdata2 */
extern const f32 ef_util_f32_100000;   /* 100000.0f                                                .sdata2 */
extern const f32 ef_util_f32_min_scale_pair[]; /* {1.0e-05f, 0.0f} - address taken                         .sdata2 */

/* The {3.0f, 0.5f} pair `ef_mtx34_column_length`'s reciprocal square root step loads with one `psq_l` (`.sdata`). */
f32 ef_util_rsqrt_three_half[2] = {3.0f, 0.5f};

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
extern "C" void ef_mtx34_from_y_axis(f32* mtx, const f32* vec) {
    f32 s;
    f32 a;
    f32 b;
    f32 z;

    NW4R_POINTER_ASSERT(mtx, 0x1AE, lbl_8059199C);
    NW4R_ASSERT(abs_f32(vec3_len(vec) - ef_util_f32_one) < ef_util_f32_min_scale, 0x1AF, lbl_805919D0);

    z = vec[2];
    s = abs_f32(z);
    if (ef_util_f32_one - s < ef_util_f32_epsilon) {
        a = ef_util_f32_zero;
        b = ef_util_f32_zero;
    } else {
        s = sqrt_f32(ef_util_f32_one - z * z);
        a = vec[1] / s;
        b = vec[0] / -s;
    }
    mtx[0] = a;
    mtx[1] = vec[0];
    mtx[2] = z * b;
    mtx[3] = ef_util_f32_zero;
    mtx[4] = b;
    mtx[5] = vec[1];
    mtx[6] = -a * z;
    mtx[7] = ef_util_f32_zero;
    mtx[8] = ef_util_f32_zero;
    mtx[9] = vec[2];
    mtx[10] = s;
    mtx[11] = ef_util_f32_zero;
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
        v20[0] = ef_util_f32_one;

    fn_8009BA78(src, 1, v14);
    if (ef_vec3_normalize_to(v14, v14) == 0)
        v14[1] = ef_util_f32_one;

    vec3_cross(v8, v20, v14);
    vec3_cross(v14, v8, v20);

    fn_8009B840(mtx, 0, v20);
    fn_8009B840(mtx, 1, v14);
    fn_8009B840(mtx, 2, v8);
    mtx[3] = ef_util_f32_zero;
    mtx[7] = ef_util_f32_zero;
    mtx[11] = ef_util_f32_zero;
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
        if (sx < ef_util_f32_flt_min)
            break;
        sy = ef_mtx34_column_length(mtx, 1);
        if (sy < ef_util_f32_flt_min)
            break;
        sz = ef_mtx34_column_length(mtx, 2);
        if (sz < ef_util_f32_flt_min)
            break;

        t = -mtx[8] / sx;
        if (t > ef_util_f32_one)
            t = ef_util_f32_one;
        if (t < ef_util_f32_minus_one)
            t = ef_util_f32_minus_one;
        rot[1] = fn_80463DE4(t);
        if (fn_8005A63C(rot[1]) >= ef_util_f32_flt_min) {
            rot[0] = atan2f(mtx[9] / sy, mtx[10] / sz);
            rot[2] = atan2f(mtx[4], mtx[0]);
        } else {
            rot[0] = atan2f(mtx[1], mtx[5]);
            rot[2] = ef_util_f32_zero;
        }
        return;
    }
    rot[0] = ef_util_f32_zero;
    rot[1] = ef_util_f32_zero;
    rot[2] = ef_util_f32_zero;
}

/* Reads the translation column of the 3x4 matrix `mtx` into `vec`. */
extern "C" void fn_8009BF08(const f32* mtx, f32* vec) {
    NW4R_POINTER_ASSERT(vec, 0x259, lbl_80591A90);

    vec[0] = mtx[3];
    vec[1] = mtx[7];
    vec[2] = mtx[11];
}

/* The asm bodies below name the table unqualified. */
using nw4r::math::sSinCosTbl;

/* 0x8009C6F0 (0x70): writes the sine and cosine of `angle` as a two-float pair at `pOut` (the table
 * lookup of `nw4r::math::SinCosFIdx`, paired-single). */
extern "C" asm void ef_vec_sin_cos(register f32* pOut, register f32 angle) {
    nofralloc
    lfs        f2, ef_util_f32_65536(r0)
    lfs        f0, ef_util_f32_rad_to_fidx(r0)
    lis        r4, sSinCosTbl@ha
    addi       r4, r4, sSinCosTbl@l
    fmuls      f0, angle, f0
    fabs       f1, f0
    psq_st     f1, 0(pOut), 1, 3
    fcmpu      cr0, f1, f2
    ble        index
reduce:
    fsubs      f1, f1, f2
    fcmpu      cr0, f1, f2
    bge        reduce
    psq_st     f1, 0(pOut), 1, 3
index:
    lhz        r0, 0(pOut)
    fsubs      f4, f2, f2
    rlwinm     r0, r0, 4, 20, 27
    add        r4, r4, r0
    psq_l      f2, 0(pOut), 1, 3
    fsubs      f2, f1, f2
    psq_l      f1, 0(r4), 0, 0
    psq_l      f3, 8(r4), 0, 0
    ps_madds0  f1, f3, f2, f1
    fcmpu      cr0, f0, f4
    bge        store
    ps_neg     f0, f1
    ps_merge01 f1, f0, f1
store:
    psq_st     f1, 0(pOut), 0, 0
    blr
}

/* 0x8009C760 (0x74): writes the sine of `angle` to `pSin` and its cosine to `pCos` (paired-single). */
extern "C" asm void ef_sin_cos(register f32* pSin, register f32* pCos, register f32 angle) {
    nofralloc
    lfs        f2, ef_util_f32_65536(r0)
    lfs        f0, ef_util_f32_rad_to_fidx(r0)
    lis        r5, sSinCosTbl@ha
    addi       r5, r5, sSinCosTbl@l
    fmuls      f0, angle, f0
    fabs       f1, f0
    psq_st     f1, 0(pSin), 1, 3
    fcmpu      cr0, f1, f2
    ble        index
reduce:
    fsubs      f1, f1, f2
    fcmpu      cr0, f1, f2
    bge        reduce
    psq_st     f1, 0(pSin), 1, 3
index:
    lhz        r0, 0(pSin)
    fsubs      f4, f2, f2
    rlwinm     r0, r0, 4, 20, 27
    add        r5, r5, r0
    psq_l      f2, 0(pSin), 1, 3
    fsubs      f2, f1, f2
    psq_l      f1, 0(r5), 0, 0
    psq_l      f3, 8(r5), 0, 0
    ps_madds0  f1, f3, f2, f1
    ps_merge10 f2, f1, f1
    psq_st     f2, 0(pCos), 1, 0
    fcmpu      cr0, f0, f4
    bge        store
    ps_neg     f1, f1
store:
    psq_st     f1, 0(pSin), 1, 0
    blr
}

/* 0x8009C7D4 (0x25C): turns the Euler angles at `rot` into the direction their rotation takes the Y axis to,
 * at `out` (the table sin/cos lookups of `ef_mtx34_rotate_xyz`, paired-single). */
extern "C" VEC3* ef_vec3_from_rotation(register const EfRotation* rot, register VEC3* out) {
    f32 work[2];
    register const u8* tbl;
    register const u8* entry0;
    register const u8* entry1;
    register f32* wk;
    register u32 idx;

    NW4R_POINTER_ASSERT(out, 0x3A6, lbl_80591B30);
    asm {
        lfs        f0, 0(rot)
        lfs        f1, 4(rot)
        lfs        f2, 8(rot)
        lis        tbl, sSinCosTbl@ha
        addi       tbl, tbl, sSinCosTbl@l
        la         idx, ef_util_f32_65536_pair(r0)
        psq_lx     f3, r0, idx, 0, 0
        lfs        f5, ef_util_f32_rad_to_fidx(r0)
        la         wk, work
        ps_merge00 f3, f3, f3
        ps_neg     f4, f3
        ps_sub     f6, f3, f3
        ps_merge00 f7, f0, f1
        ps_muls0   f7, f7, f5
        ps_abs     f0, f7
        ps_cmpu0   cr0, f0, f3
        ble        reduce_y
    reduce_x:
        ps_sum0    f0, f0, f0, f4
        ps_cmpu0   cr0, f0, f3
        bge        reduce_x
    reduce_y:
        ps_cmpu1   cr0, f0, f3
        ble        quantize_xy
        ps_merge10 f0, f0, f0
    reduce_y_loop:
        ps_sum0    f0, f0, f0, f4
        ps_cmpu0   cr0, f0, f3
        bge        reduce_y_loop
        ps_merge10 f0, f0, f0
    quantize_xy:
        psq_st     f0, 0(wk), 0, 3
        fmuls      f4, f2, f5
        psq_l      f5, 0(wk), 0, 3
        fabs       f2, f4
        fcmpu      cr0, f2, f3
        lwz        idx, 8(r1)
        ble        quantize_z
    reduce_z:
        fsubs      f2, f2, f3
        fcmpu      cr0, f2, f3
        bge        reduce_z
    quantize_z:
        psq_st     f2, 0(wk), 1, 3
        ps_sub     f5, f0, f5
        rlwinm     entry0, idx, 20, 20, 27
        add        entry0, tbl, entry0
        psq_l      f3, 0(entry0), 0, 0
        rlwinm     entry1, idx, 4, 20, 27
        psq_l      f8, 8(entry0), 0, 0
        add        entry1, tbl, entry1
        ps_madds0  f0, f8, f5, f3
        psq_l      f3, 0(entry1), 0, 0
        ps_cmpu0   cr0, f7, f6
        psq_l      f8, 8(entry1), 0, 0
        lhz        idx, 8(r1)
        bge        sign_y
        ps_neg     f9, f0
        ps_merge01 f0, f9, f0
    sign_y:
        ps_madds1  f1, f8, f5, f3
        psq_l      f5, 0(wk), 1, 3
        rlwinm     idx, idx, 4, 20, 27
        ps_cmpu1   cr0, f7, f6
        add        entry0, tbl, idx
        psq_l      f3, 0(entry0), 0, 0
        fsubs      f5, f2, f5
        psq_l      f8, 8(entry0), 0, 0
        bge        sign_z
        ps_neg     f9, f1
        ps_merge01 f1, f9, f1
    sign_z:
        ps_madds0  f2, f8, f5, f3
        fcmpu      cr0, f4, f6
        bge        build
        ps_neg     f9, f2
        ps_merge01 f2, f9, f2
    build:
        ps_muls0   f1, f1, f0
        ps_merge10 f3, f2, f2
        ps_muls1   f2, f2, f0
        ps_neg     f0, f2
        ps_merge01 f2, f0, f2
        ps_madds0  f3, f3, f1, f2
        psq_st     f3, 0(out), 0, 0
        ps_merge10 f1, f1, f1
        psq_st     f1, 8(out), 1, 0
    }
    return out;
}

/* 0x8009CA30 (0x170): builds the rotation part of the 3x4 matrix `mtx` from the Euler angles `x`, `y`, `z`
 * (radians; three table sin/cos lookups, paired-single) and zeroes its translation. */
extern "C" void ef_mtx34_rotate_xyz(register f32* mtx, register f32 x, register f32 y, register f32 z) {
    f32 work[2];

    asm {
        lis        r4, sSinCosTbl@ha
        addi       r4, r4, sSinCosTbl@l
        la         r0, ef_util_f32_65536_pair(r0)
        psq_lx     f0, r0, r0, 0, 0
        lfs        f5, ef_util_f32_rad_to_fidx(r0)
        la         r7, work
        ps_merge00 f0, f0, f0
        ps_neg     f4, f0
        ps_sub     f6, f0, f0
        ps_merge00 f7, x, y
        ps_muls0   f7, f7, f5
        ps_abs     f1, f7
        ps_cmpu0   cr0, f1, f0
        ble        reduce_y
    reduce_x:
        ps_sum0    f1, f1, f1, f4
        ps_cmpu0   cr0, f1, f0
        bge        reduce_x
    reduce_y:
        ps_cmpu1   cr0, f1, f0
        ble        quantize_xy
        ps_merge10 f1, f1, f1
    reduce_y_loop:
        ps_sum0    f1, f1, f1, f4
        ps_cmpu0   cr0, f1, f0
        bge        reduce_y_loop
        ps_merge10 f1, f1, f1
    quantize_xy:
        psq_st     f1, 0(r7), 0, 3
        fmuls      f3, z, f5
        psq_l      f4, 0(r7), 0, 3
        fabs       f2, f3
        fcmpu      cr0, f2, f0
        lwz        r0, 8(r1)
        ble        quantize_z
    reduce_z:
        fsubs      f2, f2, f0
        fcmpu      cr0, f2, f0
        bge        reduce_z
    quantize_z:
        psq_st     f2, 0(r7), 1, 3
        ps_sub     f4, f1, f4
        rlwinm     r5, r0, 20, 20, 27
        add        r5, r4, r5
        psq_l      f5, 0(r5), 0, 0
        rlwinm     r6, r0, 4, 20, 27
        psq_l      f8, 8(r5), 0, 0
        add        r6, r4, r6
        ps_madds0  f0, f8, f4, f5
        psq_l      f5, 0(r6), 0, 0
        ps_cmpu0   cr0, f7, f6
        psq_l      f8, 8(r6), 0, 0
        lhz        r0, 8(r1)
        bge        sign_y
        ps_neg     f9, f0
        ps_merge01 f0, f9, f0
    sign_y:
        ps_madds1  f1, f8, f4, f5
        psq_l      f4, 0(r7), 1, 3
        rlwinm     r0, r0, 4, 20, 27
        ps_cmpu1   cr0, f7, f6
        add        r5, r4, r0
        psq_l      f5, 0(r5), 0, 0
        fsubs      f4, f2, f4
        psq_l      f8, 8(r5), 0, 0
        bge        sign_z
        ps_neg     f9, f1
        ps_merge01 f1, f9, f1
    sign_z:
        ps_madds0  f2, f8, f4, f5
        fcmpu      cr0, f3, f6
        bge        build
        ps_neg     f9, f2
        ps_merge01 f2, f9, f2
    build:
        ps_sub     f7, f0, f0
        psq_st     f7, 44(mtx), 1, 0
        ps_neg     f3, f0
        ps_merge10 f3, f3, f0
        ps_muls0   f4, f0, f2
        ps_muls1   f5, f2, f1
        ps_merge10 f6, f5, f5
        psq_st     f6, 0(mtx), 1, 0
        ps_muls1   f6, f0, f2
        ps_muls0   f8, f3, f2
        ps_madds0  f6, f6, f1, f8
        psq_st     f6, 4(mtx), 0, 0
        ps_merge00 f6, f7, f5
        psq_st     f6, 12(mtx), 0, 0
        ps_muls1   f2, f3, f2
        ps_neg     f2, f2
        ps_madds0  f6, f4, f1, f2
        psq_st     f6, 20(mtx), 0, 0
        ps_neg     f6, f1
        ps_merge00 f6, f7, f6
        psq_st     f6, 28(mtx), 0, 0
        ps_muls1   f6, f0, f1
        psq_st     f6, 36(mtx), 0, 0
    }
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
    if (len > ef_util_f32_epsilon) {
        r = nw4r::math::FrSqrt(len);
        scale[0] = math_reciprocal(r);
        vec3_scale_by(v2c, v2c, r);

        fn_8009BA78(mtx, 1, v20);
        d0 = vec3_dot(v2c, v20);
        vec3_scale_by(v8, v2c, d0);
        PSVECSubtract(v20, v20, v8);

        len = vec3_length_sq(v20);
        if (len > ef_util_f32_epsilon) {
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
            if (len > ef_util_f32_epsilon) {
                scale[2] = sqrt_f32(len);
                vec3_cross(v8, v20, v14);
                if (vec3_dot(v2c, v8) < ef_util_f32_zero) {
                    scale[0] = scale[0] * -1.0;
                    scale[1] = scale[1] * -1.0;
                    scale[2] = scale[2] * -1.0;
                }
            } else {
                scale[2] = ef_util_f32_zero;
            }
        } else {
            scale[1] = ef_util_f32_zero;
            fn_8009BA78(mtx, 2, v14);
            d2 = vec3_dot(v2c, v14);
            vec3_scale_by(v8, v2c, d2);
            PSVECSubtract(v14, v14, v8);
            len = vec3_length_sq(v14);
            if (len > ef_util_f32_epsilon)
                scale[2] = sqrt_f32(len);
            else
                scale[2] = ef_util_f32_zero;
        }
    } else {
        scale[0] = ef_util_f32_zero;
        fn_8009BA78(mtx, 1, v20);
        len = vec3_length_sq(v20);
        if (len > ef_util_f32_epsilon) {
            r = nw4r::math::FrSqrt(len);
            scale[1] = math_reciprocal(r);
            vec3_scale_by(v20, v20, r);

            fn_8009BA78(mtx, 2, v14);
            d1 = vec3_dot(v20, v14);
            vec3_scale_by(v8, v20, d1);
            PSVECSubtract(v14, v14, v8);
            scale[2] = vec3_len(v14);
        } else {
            scale[1] = ef_util_f32_zero;
            fn_8009BA78(mtx, 2, v14);
            scale[2] = vec3_len(v14);
        }
    }
}

/* 0x8009C484 (0x26C): normalises `vec` into `dst` and returns 1, or copies `vec` unchanged and returns 0 when
 * it is the zero vector (paired-single, one reciprocal square root step). */
extern "C" s32 ef_vec3_normalize_to(register f32* dst, register const f32* vec) {
    register s32 normalized;

    NW4R_POINTER_ASSERT(dst, 0x2DB, lbl_80591A24);
    NW4R_POINTER_ASSERT(vec, 0x2DC, lbl_80591AFC);
    asm {
        lfs        f0, ef_util_f32_half(r0)
        lfs        f1, ef_util_f32_three(r0)
        psq_l      f3, 0(vec), 0, 0
        ps_mul     f6, f3, f3
        psq_l      f4, 8(vec), 1, 0
        ps_madd    f5, f4, f4, f6
        fsubs      f2, f0, f0
        ps_sum0    f5, f5, f4, f6
        fcmpu      cr0, f5, f2
        beq        zero
        frsqrte    f2, f5
        fmuls      f6, f2, f2
        fmuls      f0, f2, f0
        fnmsubs    f6, f6, f5, f1
        fmuls      f2, f6, f0
        ps_muls0   f3, f3, f2
        psq_st     f3, 0(dst), 0, 0
        ps_muls0   f4, f4, f2
        psq_st     f4, 8(dst), 1, 0
        li         normalized, 1
        b          done
    zero:
        psq_st     f3, 0(dst), 0, 0
        psq_st     f4, 8(dst), 1, 0
        li         normalized, 0
    done:
    }
    return normalized;
}

/* 0x8009CBA0 (0x80): scales every column of the 3x4 matrix `mtx` by the matching component of `scale`, into
 * `dst`; a zero component is replaced by 1.0e-05f (paired-single). */
extern "C" asm void fn_8009CBA0(register f32* dst, register const f32* mtx, register const f32* scale) {
    nofralloc
    lfs        f8, ef_util_f32_min_scale(r0)
    psq_l      f0, 0(scale), 0, 0
    ps_sub     f6, f0, f0
    psq_l      f1, 8(scale), 1, 0
    ps_cmpu0   cr0, f0, f6
    psq_l      f2, 0(mtx), 0, 0
    psq_l      f3, 8(mtx), 0, 0
    bne        y
    ps_merge01 f0, f8, f0
y:
    ps_cmpu1   cr0, f0, f6
    psq_l      f4, 16(mtx), 0, 0
    psq_l      f5, 24(mtx), 0, 0
    bne        z
    ps_merge00 f0, f0, f8
z:
    ps_cmpu0   cr0, f1, f6
    psq_l      f6, 32(mtx), 0, 0
    psq_l      f7, 40(mtx), 0, 0
    bne        scale_rows
    ps_merge01 f1, f8, f1
scale_rows:
    ps_mul     f2, f2, f0
    ps_mul     f4, f4, f0
    ps_mul     f6, f6, f0
    ps_mul     f3, f3, f1
    ps_mul     f5, f5, f1
    ps_mul     f7, f7, f1
    psq_st     f2, 0(dst), 0, 0
    psq_st     f3, 8(dst), 0, 0
    psq_st     f4, 16(dst), 0, 0
    psq_st     f6, 32(dst), 0, 0
    psq_st     f5, 24(dst), 0, 0
    psq_st     f7, 40(dst), 0, 0
    blr
}

/* 0x8009CC20 (0x8C): scales the rows of the 3x4 matrix `mtx` by the matching component of `scale`, into
 * `dst`; a zero component is replaced by 1.0e-05f (paired-single). */
extern "C" asm void ef_mtx34_scale_columns(register f32* dst, register const f32* scale, register const f32* mtx) {
    nofralloc
    lfs        f9, 0(scale)
    lis        r6, ef_util_f32_min_scale_pair@ha
    lfs        f10, 4(scale)
    addi       r6, r6, ef_util_f32_min_scale_pair@l
    lfs        f12, 0(r6)
    lfs        f11, 8(scale)
    fsubs      f13, f12, f12
    psq_l      f4, 0(mtx), 0, 0
    fcmpu      cr0, f9, f13
    psq_l      f5, 8(mtx), 0, 0
    bne        y
    fmr        f9, f12
y:
    psq_l      f6, 16(mtx), 0, 0
    fcmpu      cr0, f10, f13
    psq_l      f7, 24(mtx), 0, 0
    bne        z
    fmr        f10, f12
z:
    psq_l      f8, 32(mtx), 0, 0
    fcmpu      cr0, f11, f13
    psq_l      f2, 40(mtx), 0, 0
    bne        scale_rows
    fmr        f11, f12
scale_rows:
    ps_muls0   f4, f4, f9
    psq_st     f4, 0(dst), 0, 0
    ps_muls0   f5, f5, f9
    psq_st     f5, 8(dst), 0, 0
    ps_muls0   f6, f6, f10
    psq_st     f6, 16(dst), 0, 0
    ps_muls0   f7, f7, f10
    psq_st     f7, 24(dst), 0, 0
    ps_muls0   f8, f8, f11
    psq_st     f8, 32(dst), 0, 0
    ps_muls0   f2, f2, f11
    psq_st     f2, 40(dst), 0, 0
    blr
}

/* Builds the reciprocal of the scale vector `scale` (a zero component becomes 100000.0f) and applies it
 * to the 3x4 matrix `mtx` through fn_8050133C. */
extern "C" void fn_8009CCAC(f32* dst, const f32* mtx, const f32* scale) {
    f32 inv[3];

    VEC3_ctor((nw4r::math::VEC3*)inv);
    if (ef_util_f32_zero != scale[0])
        inv[0] = math_reciprocal(scale[0]);
    else
        inv[0] = ef_util_f32_100000;
    if (ef_util_f32_zero != scale[1])
        inv[1] = math_reciprocal(scale[1]);
    else
        inv[1] = ef_util_f32_100000;
    if (ef_util_f32_zero != scale[2])
        inv[2] = math_reciprocal(scale[2]);
    else
        inv[2] = ef_util_f32_100000;
    fn_8050133C(dst, mtx, inv);
}

/* 0x8009CD64 (0x58): returns the length of column `index` of the 3x4 matrix `mtx` (paired-single, with one
 * reciprocal square root step). */
extern "C" asm f32 ef_mtx34_column_length(register const f32* mtx, register s32 index) {
    nofralloc
    slwi       index, index, 2
    add        r4, mtx, index
    psq_l      f0, 0(r4), 1, 0
    psq_l      f1, 16(r4), 1, 0
    ps_merge00 f0, f0, f1
    ps_mul     f0, f0, f0
    lfs        f1, 32(r4)
    fsubs      f2, f0, f0
    ps_madd    f1, f1, f1, f0
    ps_sum0    f1, f1, f0, f0
    fcmpu      cr0, f1, f2
    beqlr
    frsqrte    f0, f1
    lis        r6, ef_util_rsqrt_three_half@ha
    addi       r6, r6, ef_util_rsqrt_three_half@l
    psq_l      f3, 0(r6), 0, 0
    fmuls      f2, f0, f0
    ps_muls1   f0, f0, f3
    fnmsubs    f2, f2, f1, f3
    fmuls      f0, f2, f0
    fmuls      f1, f1, f0
    blr
}
