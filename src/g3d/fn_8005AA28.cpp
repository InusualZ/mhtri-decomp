/*
 * nw4r g3d: the `ResMat` accessor cluster at 0x8005AA28-0x8005ABD8 (8 functions).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit over
 * config/RMHE08/symbols.txt: every 0x8005AA28..0x8005ABD8 row is a bare `fn_XXXXXXXX`).
 *
 * Module `g3d` is class-1 evidence, the file name is class 4.  The range's only `__FILE__` string is
 * `lbl_8058B370` = "g3d_resnode_ac.h" (with `lbl_8058B350` = "NW4R:Failed assertion IsValid()"), the
 * `pFile`/`pFmt` pair of fn_8005AA44's `nw4r::db::Panic`, read out of orig/RMHE08/sys/main.dol's `.data`.
 * It names the `_ac.h` accessor header the assert was written in, not this translation unit, and the map
 * carries no `__FILE__` `.cpp` string for the range, so the file keeps the map's `fn_` stem (docs/plan.md
 * 12, class 4).  The module is g3d because every caller is nw4r g3d: the constructor
 * `nw4r::g3d::ScnMdl::CopiedMatAccess::CopiedMatAccess` calls fn_8005AB00/fn_8005AA30, g3d_calcworld.cpp's
 * fn_80073E8C repeats this file's `IsValid()` assert shape against fn_8005AAEC, and g3d_basic.cpp's
 * fn_800D7ED0 is fn_8005AB08's twin (the same fn_80050508/fn_80051570 pair, PSMTXScaleApply for
 * PSMTXTransApply).
 *
 * The eight are the out-of-line copies of the `_ac.h` accessors: the resource pointer (fn_8005AAE4), two
 * `IsValid()` copies (fn_8005AA30/fn_8005AAEC), the type-name string (fn_8005AA28), the draw-order word
 * (fn_8005AB00), the assert-then-set/clear flag helper (fn_8005AA44), the matrix translate (fn_8005AB08)
 * and the two file-scope VEC3's static initialiser (fn_8005AB78, the `.ctors` word 0x8056F2D0).
 *
 * Measured per symbol against the retired `build/RMHE08/obj/auto_*_text.o` split targets (the unit has no
 * ninja rule or split object in MAIN yet, so `recompile.py` fell back to the run objects that own each
 * address): all 8 symbols are byte-identical, 100.0 % -
 *   fn_8005AA28 (8 B), fn_8005AA30 (20 B), fn_8005AA44 (160 B), fn_8005AAE4 (8 B), fn_8005AAEC (20 B),
 *   fn_8005AB00 (8 B), fn_8005AB08 (112 B), fn_8005AB78 (96 B).
 * The object's section sizes match the claim: extab 0x18, extabindex 0x24.  The `.ctors` word 0x8056F2D0
 * is claimed because dtk's split assigns it to fn_8005AB78 (the same rule that gave mh3_pad.cpp's and
 * draw_shape.cpp's words to them); our object writes the retail static initialiser as the plain function
 * it compiles to, so it does not emit the word itself - a residual for the link gate, not a measurement.
 */
#include "types.h"
#include "nw4r/math.h"

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling
 * (docs/plan.md 6.5 rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The g3d resource handle the `_ac.h` accessors read: the resource pointer at +0x0 (the shared
 * `ResCommon<T>` one-word idiom) and the mat's draw-order word at +0xE8.
 * size: 0xEC (only these two fields are reached; a lower bound) */
struct ResMatHandle {
    /* +0x00 */ void* mpRes;
    /* +0x04 */ u8 pad_0x04[0xE4];
    /* +0xE8 */ u32 mDrawOrder;
};

/* The resource block `mpRes` names: fn_8005AA44 sets/clears bit 0x100 of the mat's flag word at +0x14.
 * size: 0x18 (only this field is reached; a lower bound) */
struct ResMatData {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u32 mFlags;
};

/* The pooled constants and strings this unit reads; they live in the original `.data`/`.bss`/`.sdata2`,
 * which the unit does not claim, so they are declarations here.  Each is still unsplit in the map (its
 * address band interleaves modules), so the declaration has no registered owner to move to. */
extern "C" {
extern const char lbl_8058B350[]; /* "NW4R:Failed assertion IsValid()"            .data */
extern const char lbl_8058B370[]; /* "g3d_resnode_ac.h"                          .data */
extern Vec3 lbl_8066AE48;         /* the first file-scope VEC3 (0, 0, 0)         .bss  */
extern Vec3 lbl_8066AE54;         /* the second file-scope VEC3 (100, 0, 0)      .bss  */
extern f32 lbl_80795CF0;          /* 0.0f                                       .sdata2 */
extern f32 lbl_80795CF4;          /* 100.0f                                     .sdata2 */
}

/* The neighbours this unit calls.  Their registered owners are named beside each; the prototypes sit in
 * this `extern "C"` block (the same shape g3d_basic.cpp uses) so the rule-2 conformance pass can lift
 * them into the owners' headers.  These are `fn_XXXXXXXX`/SDK stems, not manglings, so rule 9 does not
 * reach them. */
extern "C" {
void fn_80041E8C(Vec3* pOut, f32 x, f32 y, f32 z); /* owner: src/mh3_pad.cpp            */
void* fn_80050508(void* pMtx);                     /* owner: src/fn_8004CAD8.cpp        */
void* fn_80051570(void* pMtx);                     /* owner: src/fn_8004CAD8.cpp        */
/* `PSMTXTransApply(const Mtx src, Mtx dst, f32 x, f32 y, f32 z)` - the SDK math helper (the declaration
 * g3d_basic.cpp carries for its PSMTXScaleApply twin). */
void PSMTXTransApply(Mtx34* pDst, const Mtx34* pSrc, f32 x, f32 y, f32 z);
}

extern "C" {

/* Forward declarations for the two `_ac.h` accessors fn_8005AA44 re-checks after its assert. */
void* fn_8005AAE4(const ResMatHandle* pSelf);
u32 fn_8005AAEC(const ResMatHandle* pSelf);

/* The `ResMat` type-name accessor: the resource's own name string. */
const char* fn_8005AA28(void) {
    return "ResMat";
}

/* `IsValid()`: the resource pointer is non-null.  The `neg`/`or`/`srwi` word test is MWCC's `!= 0`. */
u32 fn_8005AA30(const ResMatHandle* pSelf) {
    return pSelf->mpRes != NULL;
}

/* The `_ac.h` flag setter: assert the handle is valid, then set or clear the mat's 0x100 flag. */
void fn_8005AA44(ResMatHandle* pSelf, u32 enable) {
    if (!fn_8005AAEC(pSelf)) {
        nw4r::db::Panic(lbl_8058B370, 0xAF, lbl_8058B350);
    }
    if (fn_8005AAEC(pSelf)) {
        if (enable != 0) {
            ((ResMatData*)fn_8005AAE4(pSelf))->mFlags |= 0x100;
        } else {
            ((ResMatData*)fn_8005AAE4(pSelf))->mFlags &= ~0x100;
        }
    }
}

/* `ptr()`: the resource pointer the handle stores. */
void* fn_8005AAE4(const ResMatHandle* pSelf) {
    return pSelf->mpRes;
}

/* A second `IsValid()` out-of-line copy, byte-identical to fn_8005AA30 (a different `_ac.h` inline). */
u32 fn_8005AAEC(const ResMatHandle* pSelf) {
    return pSelf->mpRes != NULL;
}

/* The mat's draw-order word (`ScnMdl::CopiedMatAccess` feeds it to its material lookup). */
u32 fn_8005AB00(const ResMatHandle* pSelf) {
    return pSelf->mDrawOrder;
}

/* The `PSMTXTransApply` twin of g3d_basic.cpp's fn_800D7ED0: translate `pPos` into the node matrix and
 * return `pSrc`. */
Mtx34* fn_8005AB08(Mtx34* pSrc, const Vec3* pPos, void* pNodeMtx) {
    const Mtx34* src = (const Mtx34*)fn_80050508(pSrc);
    Mtx34* dst = (Mtx34*)fn_80051570(pNodeMtx);
    PSMTXTransApply(dst, src, pPos->x, pPos->y, pPos->z);
    return pSrc;
}

/* The static initialiser of the two file-scope VEC3s (the `.ctors` word 0x8056F2D0):
 *   Vec3 lbl_8066AE48(0.0f, 0.0f, 0.0f);
 *   Vec3 lbl_8066AE54(100.0f + lbl_8066AE48.x, lbl_8066AE48.y, lbl_8066AE48.z);
 * Written as the out-of-line function it compiles to, so the map's fn_8005AB78 stays its name. */
void fn_8005AB78(void) {
    fn_80041E8C(&lbl_8066AE48, lbl_80795CF0, lbl_80795CF0, lbl_80795CF0);
    fn_80041E8C(&lbl_8066AE54, lbl_80795CF4 + lbl_8066AE48.x, lbl_8066AE48.y, lbl_8066AE48.z);
}

}  // extern "C"
