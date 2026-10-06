/*
 * g3d/fn_8005AA28.cpp - nw4r g3d `ResMat` accessor copies of the `_ac.h` inlines and a VEC3 static initialiser.
 * RANGE. .text 0x8005AA28-0x8005ABD8 (8 functions); extab, extabindex, .ctors 0x8056F2D0-0x8056F2D4 (fn_8005AB78's
 *   word), .data 0x8058B350-0x8058B388, .bss 0x8066AE48-0x8066AE60, .sdata 0x80791124-0x80791130, .sdata2
 *   0x80795CF0-0x80795CF8.
 * NAMES. The file keeps the map's stem: the range's only `__FILE__` string is the accessor header
 *   "g3d_resnode_ac.h" (lbl_8058B370, fn_8005AA44's assert).  Module `g3d` from the callers
 *   (`nw4r::g3d::ScnMdl::CopiedMatAccess`, `g3d/g3d_calcworld.cpp`'s fn_80073E8C; mtx34_trans_apply's twin is
 *   `g3d/g3d_basic.cpp`'s fn_800D7ED0).
 *   GUESS: `mtx34_trans_apply` (the `PSMTXTransApply` wrapper at 0x8005AB08).
 * RESIDUALS. none in `.text`.  flipcheck: `.ctors` (fn_8005AB78 compiles as a plain function), `.data`, `.bss` and
 *   `.sdata2` are claimed and not emitted; `.sdata` is 0x7 of 0xC.
 */
#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMat (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple (rule 2) */

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

/* The pooled constants and strings this unit reads, declared, not defined: the claimed `.data`/`.bss`/`.sdata2`
 * are not emitted yet. */
extern "C" {
extern const char lbl_8058B350[]; /* "NW4R:Failed assertion IsValid()"            .data */
extern const char lbl_8058B370[]; /* "g3d_resnode_ac.h"                          .data */
extern Vec3 lbl_8066AE48;         /* the first file-scope VEC3 (0, 0, 0)         .bss  */
extern Vec3 lbl_8066AE54;         /* the second file-scope VEC3 (100, 0, 0)      .bss  */
extern f32 lbl_80795CF0;          /* 0.0f                                       .sdata2 */
extern f32 lbl_80795CF4;          /* 100.0f                                     .sdata2 */
}

/* The neighbours this unit calls (plain map stems), each owner named beside it. */
extern "C" {
void* mtx34_get_ptr(void* pMtx);                     /* owner: src/fn_8004CAD8.cpp        */
void* mtx34_const_ptr(void* pMtx);                     /* owner: src/fn_8004CAD8.cpp        */
/* `PSMTXTransApply(const Mtx src, Mtx dst, f32 x, f32 y, f32 z)` - the SDK math helper (the declaration
 * g3d_basic.cpp carries for its PSMTXScaleApply twin). */
void PSMTXTransApply(Mtx34* pDst, const Mtx34* pSrc, f32 x, f32 y, f32 z);
}

extern "C" {

/* Forward declarations for the two `_ac.h` accessors fn_8005AA44 re-checks after its assert. */
void* fn_8005AAE4(const ResMatHandle* pSelf);
u32 fn_8005AAEC(const ResMatHandle* pSelf);

} /* extern "C" */

/* 0x8005AA28 (0x8): returns the class name. */
const char* nw4r::g3d::ResMat::GetClassName() {
    return "ResMat";
}

/* 0x8005AA30 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMat::IsValid() const {
    return mpData != NULL;
}

extern "C" {

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

}   /* extern "C": the ScnMdlSimple member below has C++ linkage */

/* 0x8005AB00 (0x8): the model resource. */
nw4r::g3d::ResMdl nw4r::g3d::ScnMdlSimple::GetResMdl()
{
    return mResMdl;
}

extern "C" {

/* The `PSMTXTransApply` twin of g3d_basic.cpp's fn_800D7ED0: translate `pPos` into the node matrix and
 * return `pSrc`. */
Mtx34* mtx34_trans_apply(Mtx34* pSrc, const Vec3* pPos, void* pNodeMtx) {
    const Mtx34* src = (const Mtx34*)mtx34_get_ptr(pSrc);
    Mtx34* dst = (Mtx34*)mtx34_const_ptr(pNodeMtx);
    PSMTXTransApply(dst, src, pPos->x, pPos->y, pPos->z);
    return pSrc;
}

/* Constructs the two file-scope VEC3s (the `.ctors` word 0x8056F2D0): lbl_8066AE48 = (0, 0, 0) and
 * lbl_8066AE54 = lbl_8066AE48 + (100, 0, 0). */
void fn_8005AB78(void) {
    setVec3(&lbl_8066AE48, lbl_80795CF0, lbl_80795CF0, lbl_80795CF0);
    setVec3(&lbl_8066AE54, lbl_80795CF4 + lbl_8066AE48.x, lbl_8066AE48.y, lbl_8066AE48.z);
}

}  // extern "C"
