/* ef_util.cpp - the 16 functions at `.text` 0x8009B374..0x8009CD64 (6640 B) of the discovery
 * proposal `8009B374_fn_8009B374`.
 *
 * Naming - which evidence class decided it.  Class 1 decides: the range's own `.data` pool holds the
 * bare source-file name `ef_util.cpp` (lbl_80591948 at 0x80591948, read out of orig/RMHE08/sys/main.dol),
 * the `pFile` argument of the `nw4r::db::Panic` asserts fn_8009B374/fn_8009B448/fn_8009B650/fn_8009BCB4/
 * fn_8009BF08/fn_8009C040/fn_8009C484/fn_8009C7D4 reach; the `.cpp` suffix and the
 * `Panic__Q24nw4r2dbFPCciPCce` relocation both say C++ (langcheck's evidence kinds `source-cpp` and
 * `mangled-undefined`).  Module `ef`: the file name is the module's own, the sibling `src/ef/*.cpp` units
 * are the NW4R effect library this file belongs to, and the module's lib (`ef`, cflags_main) is the one
 * its family uses - its flags are token-identical to the neighbour `g3d/g3d_gpu.cpp`'s `cflags_g3d`, so
 * the lib choice cannot change the codegen.  Class 2 fails: `python tools/symbols/dumpmap.py lookup
 * 0x8009B374` (and every other address in the range) answers the `zz_009b374_` placeholders, not a name.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` over all 17 symbols, which answer `zz_` placeholders,
 * and with `python tools/symbols/symedit.py range 0x8009B374 0x8009CDBC`, whose every entry is a bare
 * `fn_XXXXXXXX`).  The map's stems stand and are used as the identifiers.
 *
 * Section claim.  `.text` 0x8009B374..0x8009CD64, `extab` 0x80009A38..0x80009A98 (the twelve 8-byte
 * unwind-only records `@etb_80009A38`..`@etb_80009A90`) and `extabindex` 0x80022B30..0x80022BC0 (the
 * twelve 0xC-byte entries `@eti_80022B30`..`@eti_80022BB4`); both runs abut the neighbouring
 * registrations (g3d/g3d_gpu.cpp claims extab ..0x80009A38 and extabindex ..0x80022B30, and the next
 * original TU's first entry is at 0x80009A98/0x80022BC0), so the ranges are gapless and both ends are
 * entry boundaries.  The `.data` strings, the `.sdata2` pool (0x80795F70..0x80795FB8) and the `.rodata`
 * sin/cos table (0x80573CD8) the range references are *not* claimed: they stay in the unclaimed pool,
 * which is how the sibling `g3d/g3d_gpu.cpp` treats its own, and playbook 23's regression is exactly a
 * pool claimed by a unit whose object does not emit it.
 *
 * Seam - what the range's own evidence settles, and what it does not.  Both ends are evidence-bounded,
 * not the brief's byte cap.  The *left* edge is the `__FILE__` anchor: fn_8009B140 (the unit that landed
 * before this one) cites `.data` 0x80591900 = "g3d_gpu.cpp" while every one of this range's first bodies
 * cites 0x80591948 = "ef_util.cpp" (two bare source names = two TUs), and the private `.sdata2` pool
 * word 0x80795F6C (a scalar 0.0f referenced only by fn_8009B2CC) is a *second*, separate 0.0f from this
 * range's own 0x80795F7C (referenced by five of these bodies) - MWCC pools a constant once per TU, so
 * 0x80795F6C and 0x80795F7C cannot be the same TU's pool.  The brief's own cap started at 0x8009B374,
 * which is where the landed unit's range ends, so the non-overlapping part is this unit.  The *right*
 * edge is fn_8009CD64: the pool run of this TU ends at 0x80795FB8 (fn_8009CDBC, the next original TU's
 * first body, cites it together with "ef_animcurve.cpp" at 0x80591E68), and fn_8009CD64 itself
 * references neither - its only constant is the `.sdata` pair {3.0f, 0.5f} at 0x807912D8, a fragment
 * linked before this one's.  The brief's end 0x8009CDBC is one function past the TU boundary.
 *
 * What the bodies do.  This is the NW4R effect library's shared math/utility file: the `nw4r::ut::List`
 * to array copy, the VEC3/MTX34 helpers (rotation basis, sin/cos pairs, column length, scale/axis
 * matrix builders) and the pointer-assert macro the whole `ef` band uses.
 *
 * Reconstruction status (official report metric, `recompile.py --measure <symbol>` against MAIN's
 * retired per-symbol/run split objects - the unit is registered for the first time, so MAIN has no split
 * object for the range yet).  Nine of the sixteen bodies are reconstructed, all at or above the bar:
 * fn_8009B374 100.00, fn_8009B650 100.00, fn_8009B840 100.00, fn_8009BA78 100.00, fn_8009BF08 100.00,
 * fn_8009B448 99.46 (520/520 B, register colours on two rows), fn_8009BCB4 96.78, fn_8009CCAC 95.00,
 * fn_8009C040 94.27 (frame 0x50 vs 0x60 and one register tie-break).
 *
 * The other seven are the campaign's **paired-single class** (`docs/matching.md`, "A paired-single
 * instruction names a function that was not built from this C frontend"): fn_8009C6F0 (19 ps ops),
 * fn_8009C760 (19), fn_8009C7D4 (46), fn_8009CA30 (60), fn_8009CBA0 (45), fn_8009CC20 (45) - 176 of the game's 19,916 functions carry ps body ops and **none** matches, across
 * all 33 compilers under build/compilers with every relevant flag combination (the retail `.comment`
 * byte is 0x0e where every available compiler writes 0x0f).  The C reconstructions of those bodies are
 * kept where they are semantically clear (fn_8009C484 62.32, fn_8009C760 30.69, fn_8009C6F0 26.61 and
 * the three scale/column helpers at 0-2 %), and the two whose whole body is the ps sequence
 * (fn_8009C7D4, fn_8009CA30) are stubs with the residual recorded above them.  No flag change is
 * requested for this: the class is a compiler-build gap, not a command-line one.
 *
 * Two per-function pragma regions were needed, both measured (playbook 39/40): fn_8009B374 and
 * fn_8009B448 keep the **unfused** forms retail has (`clrlwi`+`slwi` where the peephole fuses
 * `clrlslwi`, `li r0,<slot>; psq_lx` for the FPR epilogue, and `fmuls`+`fsubs` where `-fp_contract`
 * fuses `fnmsubs`), so those two bodies sit between `#pragma peephole off`/`#pragma fp_contract off`
 * and their resets.  The resets are immediately after fn_8009B448: a file-wide `peephole off` costs
 * fn_8009C6F0 9.6 and fn_8009C760 9.3 points (measured), and the other nine bodies are byte-identical
 * with the pass on.
 *
 * Shared-file note: this unit's declarations for the symbols `src/fn_8004CAD8.cpp` owns
 * (fn_80050EDC/fn_80050F24/fn_80050BC0/fn_80051424/fn_80051820/fn_80052214/PSVECSubtract) were added to
 * that owner's header `include/fn_8004CAD8.h` (rule 2), because the owner registered before this unit
 * and its header carried only the five declarations its own consumers needed.
 */

#include "types.h"
#include "fn_8004CAD8.h"    /* the VEC3 helpers this range owns (rule 2) */
#include "mh3_pad.h"        /* VEC3_ctor - owner src/mh3_pad.cpp (rule 2) */
#include "g3d/g3d_anmchr.h" /* fn_800610AC - owner src/g3d/g3d_anmchr.cpp (rule 2) */

/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map's name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling
 * (rule 9).  nw4r::db is unsplit, so the declaration lives with its consumers. */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db

namespace math {

/* `FrSqrt__Q24nw4r4mathFf` - the reciprocal square root the scale path uses (rule 9: the owner is the
 * namespace, never the mangled spelling).  nw4r::math is unsplit, so it is declared here. */
f32 FrSqrt(f32 x);

} // namespace math
} // namespace nw4r

/* The not-yet-reconstructed helpers this range reaches: their address bands do not name a module (the
 * registered bands interleave there), so the declaration stays with the consumer (rule 2's named gap). */
extern "C" {
void* fn_80501C60(void* list, void* node);
/* 0x8009CD64 - the next original TU's column-length helper, reached by fn_8009BCB4; unsplit, and its
 * address band does not name a module, so the declaration stays with the consumer (rule 2's named gap). */
f32 fn_8009CD64(const f32* mtx, s32 index);
f32 fn_80463DE4(f32 x);
f32 fn_80463E08(f32 y, f32 x);
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
s32 fn_8009C484(f32* dst, const f32* src);
void fn_8009C7D4(f32* mtx, const f32* a, const f32* b);
void fn_8009CA30(f32* mtx, f32 x, f32 y, f32 z);
void fn_8009CCAC(f32* dst, const f32* mtx, const f32* scale);
void fn_8050133C(f32* dst, const f32* mtx, const f32* scale);
}

/* The file's own pooled strings, the pooled floats and the sin/cos table: referenced but never defined
 * here (rule 8.4 - the unit does not claim the `.data`/`.sdata2`/`.rodata` pools). */
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
extern const f32 lbl_80573CD8[]; /* the 256-entry x 4-float sin/cos interpolation table      .rodata */

/* The library's pointer assert: `ptr` must fall in one of the seven mapped memory ranges.  The six
 * materialised BOOLs and the two-test first `if` are the target's exact shape - the same expansion the
 * siblings `src/ef/ef_point.cpp`, `src/ef/ef_line.cpp` and `src/g3d/g3d_gpu.cpp` needed.  The trailing
 * `(ptr)` is the target's fourth argument to the variadic `Panic` (`mr r6, <ptr>`). */
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

/* The target keeps the unfused `clrlwi`+`slwi` pair and schedules the count's increment before the
 * store, which is the peephole pass's fused `clrlslwi` form - so this one body needs the pass off
 * (playbook 39).  The reset is immediately after it: the rest of the unit is byte-identical with the
 * pass on (measured - the sibling bodies below lose 4-9 points under a file-wide pragma). */
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
    NW4R_ASSERT(fn_8005220C(fn_80050F24(vec) - lbl_80795F80) < lbl_80795F84, 0x1AF, lbl_805919D0);

    z = vec[2];
    s = fn_8005220C(z);
    if (lbl_80795F80 - s < lbl_80795F8C) {
        a = lbl_80795F7C;
        b = lbl_80795F7C;
    } else {
        s = fn_80050BC0(lbl_80795F80 - z * z);
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

    VEC3_ctor(v20);
    VEC3_ctor(v14);
    VEC3_ctor(v8);

    fn_8009BA78(src, 0, v20);
    if (fn_8009C484(v20, v20) == 0)
        v20[0] = lbl_80795F80;

    fn_8009BA78(src, 1, v14);
    if (fn_8009C484(v14, v14) == 0)
        v14[1] = lbl_80795F80;

    fn_80051820(v8, v20, v14);
    fn_80051820(v14, v8, v20);

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
        sx = fn_8009CD64(mtx, 0);
        if (sx < lbl_80795F90)
            break;
        sy = fn_8009CD64(mtx, 1);
        if (sy < lbl_80795F90)
            break;
        sz = fn_8009CD64(mtx, 2);
        if (sz < lbl_80795F90)
            break;

        t = -mtx[8] / sx;
        if (t > lbl_80795F80)
            t = lbl_80795F80;
        if (t < lbl_80795F88)
            t = lbl_80795F88;
        rot[1] = fn_80463DE4(t);
        if (fn_8005A63C(rot[1]) >= lbl_80795F90) {
            rot[0] = fn_80463E08(mtx[9] / sy, mtx[10] / sz);
            rot[2] = fn_80463E08(mtx[4], mtx[0]);
        } else {
            rot[0] = fn_80463E08(mtx[1], mtx[5]);
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
    tbl = lbl_80573CD8 + (*(u16*)pOut & 0xFF) * 4;
    a = a - pOut[0];
    s = tbl[0] + tbl[2] * a;
    c = tbl[1] + tbl[3] * a;
    if (y < lbl_80795F7C)
        s = -s;
    pOut[0] = s;
    pOut[1] = c;
}

/* Writes the sine of `angle` to `pSin` and its cosine to `pCos`. */
extern "C" void fn_8009C760(f32* pSin, f32* pCos, f32 angle) {
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
    tbl = lbl_80573CD8 + (*(u16*)pSin & 0xFF) * 4;
    a = a - pSin[0];
    s = tbl[0] + tbl[2] * a;
    c = tbl[1] + tbl[3] * a;
    pCos[0] = c;
    if (y < lbl_80795F7C)
        s = -s;
    pSin[0] = s;
}

/* Builds a rotation matrix from three Euler angles (the paired-single sibling of fn_8009C6F0, sharing
 * the table lookup and the reduce-to-65536 loops).
 *
 * Residual, recorded rather than hunted: the target's 604-byte body is 46 paired-single instructions
 * (`psq_lx`/`psq_st`/`ps_mul`/`ps_madds0`/`ps_merge`/`ps_sum0`/`ps_cmpu`/`ps_neg`) and the ps sequence
 * *is* the function - the only non-paired part is the leading pointer assert.  docs/matching.md's
 * "A paired-single instruction names a function that was not built from this C frontend" measures the
 * class over the whole game: 176 of 19,916 functions carry ps ops and none of them matches, across all
 * 33 compilers in build/compilers with every relevant flag combination (the retail `.comment` byte is
 * 0x0e where every available compiler writes 0x0f).  This body is a stub so the symbol stays in the
 * object; the assert and the algorithm are described above for whoever revisits the class. */
extern "C" void fn_8009C7D4(f32* mtx, const f32* a, const f32* b) {
}

/* Builds a rotation matrix from three Euler angles (the paired-single sibling of fn_8009C760).
 *
 * Residual, recorded rather than hunted: the target's 368-byte body is 60 paired-single instructions
 * (`ps_merge00`/`ps_muls0`/`ps_abs`/`ps_sum0`/`ps_cmpu`/`psq_l`/`psq_st`/`ps_madds0`/`ps_madds1`/
 * `ps_merge10`/`ps_neg`), i.e. the whole function is the ps sequence - there is not even an assert to
 * reconstruct.  Same measured class as fn_8009C7D4 above. */
extern "C" void fn_8009CA30(f32* mtx, f32 x, f32 y, f32 z) {
}

/* Computes the three scale factors of the 3x4 matrix `mtx` into `scale`: each column's length after the
 * previous columns have been projected out (Gram-Schmidt), with a handedness sign on all three when the
 * basis is left-handed.  A column that collapses to zero contributes a zero scale. */
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

    VEC3_ctor(v8);
    VEC3_ctor(v14);
    VEC3_ctor(v20);
    VEC3_ctor(v2c);

    fn_8009BA78(mtx, 0, v2c);
    len = fn_80050EDC(v2c);
    if (len > lbl_80795F8C) {
        r = nw4r::math::FrSqrt(len);
        scale[0] = fn_800610AC(r);
        fn_80051424(v2c, v2c, r);

        fn_8009BA78(mtx, 1, v20);
        d0 = fn_80052214(v2c, v20);
        fn_80051424(v8, v2c, d0);
        PSVECSubtract(v20, v20, v8);

        len = fn_80050EDC(v20);
        if (len > lbl_80795F8C) {
            r = nw4r::math::FrSqrt(len);
            scale[1] = fn_800610AC(r);
            d0 = d0 * r;
            fn_80051424(v20, v20, r);

            fn_8009BA78(mtx, 2, v14);
            d1 = fn_80052214(v20, v14);
            fn_80051424(v8, v20, d1);
            PSVECSubtract(v14, v14, v8);
            d2 = fn_80052214(v2c, v14);
            fn_80051424(v8, v2c, d2);
            PSVECSubtract(v14, v14, v8);

            len = fn_80050EDC(v14);
            if (len > lbl_80795F8C) {
                scale[2] = fn_80050BC0(len);
                fn_80051820(v8, v20, v14);
                if (fn_80052214(v2c, v8) < lbl_80795F7C) {
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
            d2 = fn_80052214(v2c, v14);
            fn_80051424(v8, v2c, d2);
            PSVECSubtract(v14, v14, v8);
            len = fn_80050EDC(v14);
            if (len > lbl_80795F8C)
                scale[2] = fn_80050BC0(len);
            else
                scale[2] = lbl_80795F7C;
        }
    } else {
        scale[0] = lbl_80795F7C;
        fn_8009BA78(mtx, 1, v20);
        len = fn_80050EDC(v20);
        if (len > lbl_80795F8C) {
            r = nw4r::math::FrSqrt(len);
            scale[1] = fn_800610AC(r);
            fn_80051424(v20, v20, r);

            fn_8009BA78(mtx, 2, v14);
            d1 = fn_80052214(v20, v14);
            fn_80051424(v8, v20, d1);
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
extern "C" s32 fn_8009C484(f32* dst, const f32* vec) {
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
extern "C" void fn_8009CC20(f32* dst, const f32* scale, const f32* mtx) {
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

    VEC3_ctor(inv);
    if (lbl_80795F7C != scale[0])
        inv[0] = fn_800610AC(scale[0]);
    else
        inv[0] = lbl_80795FAC;
    if (lbl_80795F7C != scale[1])
        inv[1] = fn_800610AC(scale[1]);
    else
        inv[1] = lbl_80795FAC;
    if (lbl_80795F7C != scale[2])
        inv[2] = fn_800610AC(scale[2]);
    else
        inv[2] = lbl_80795FAC;
    fn_8050133C(dst, mtx, inv);
}
