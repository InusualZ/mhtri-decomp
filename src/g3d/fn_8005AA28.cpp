/*
 * g3d/fn_8005AA28.cpp - nw4r g3d `ResMat` accessor copies of the `_ac.h` inlines and a VEC3 static initialiser.
 * RANGE. .text 0x8005AA28-0x8005ABD8 (8 functions); extab, extabindex, .ctors 0x8056F2D0-0x8056F2D4 (the static
 *   initialiser's word), .data 0x8058B350-0x8058B388, .bss 0x8066AE48-0x8066AE60, .sdata 0x80791124-0x80791130,
 *   .sdata2 0x80795CF0-0x80795CF8.
 * FLAGS. `cflags_g3d`; `#pragma pool_data off`.
 * NAMES. res_handle_ptr is a GUESS (the evidence follows).
 *   res_node_is_valid is a GUESS (the evidence follows).
 *   The file keeps the map's stem: the range's only `__FILE__` string is the accessor header
 *   "g3d_resnode_ac.h" (fn_8005AA44's assert).  Module `g3d` from the callers
 *   (`nw4r::g3d::ScnMdl::CopiedMatAccess`, `g3d/g3d_calcworld.cpp`'s fn_80073E8C; mtx34_trans_apply's twin is
 *   `g3d/g3d_basic.cpp`'s fn_800D7ED0).
 *   GUESS: `mtx34_trans_apply` (the `PSMTXTransApply` wrapper at 0x8005AB08).
 *   GUESS: res_mat_set_flag_0x100 (the dead-stripped caller of fn_8005AA44), s_origin / s_origin_x100 (map rows
 *   lbl_8066AE48 / lbl_8066AE54), StaticVec3.  The static initialiser is MWCC's `__sinit_\fn_8005AA28_cpp`
 *   (map row fn_8005AB78).
 * RESIDUALS. none in code or data once the map rows carry the names above; the extra dead-stripped function
 *   makes flipcheck read .text/extab as different, while the linked DOL is byte-identical.
 * SHAPES. The data is the TU's own: fn_8005AA44 is an `inline` (`_ac.h`) function referenced by the
 *   dead-stripped res_mat_set_flag_0x100, so its two strings are weak `@STRING@` objects after the TU's own
 *   (the inline tail, playbook 80); "ResMat" (7 bytes) goes to `.sdata`; the two VEC3s are file-scope statics
 *   (`.bss`) whose constructors call `setVec3`, which makes MWCC emit `__sinit`, its `.ctors` word and the
 *   0.0f/100.0f `.sdata2` pool.  The (base, dx) constructor reproduces retail's argument order (`this`, then
 *   the x sum, then y/z through the base reference).
 */
#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMat (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple (rule 2) */

#pragma pool_data off

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
void fn_8005AA44(ResMatHandle* pSelf, u32 enable);
void* res_handle_ptr(const ResMatHandle* pSelf);
u32 res_node_is_valid(const ResMatHandle* pSelf);

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

/* Sets the mat's 0x100 flag through the `_ac.h` setter.  Nothing calls it, so the linker strips it, as it
 * stripped retail's caller; it is what makes MWCC emit fn_8005AA44 here as an inline copy. */
void res_mat_set_flag_0x100(ResMatHandle* pSelf)
{
    fn_8005AA44(pSelf, 1);
}

/* The `_ac.h` flag setter: assert the handle is valid, then set or clear the mat's 0x100 flag. */
inline void fn_8005AA44(ResMatHandle* pSelf, u32 enable) {
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic("g3d_resnode_ac.h", 0xAF, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf)) {
        if (enable != 0) {
            ((ResMatData*)res_handle_ptr(pSelf))->mFlags |= 0x100;
        } else {
            ((ResMatData*)res_handle_ptr(pSelf))->mFlags &= ~0x100;
        }
    }
}

/* `ptr()`: the resource pointer the handle stores. */
void* res_handle_ptr(const ResMatHandle* pSelf) {
    return pSelf->mpRes;
}

/* A second `IsValid()` out-of-line copy, byte-identical to fn_8005AA30 (a different `_ac.h` inline). */
u32 res_node_is_valid(const ResMatHandle* pSelf) {
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

}  // extern "C"

/* A file-scope vector whose constructors build it through `setVec3`.  size: 0xC */
struct StaticVec3 : public Vec3 {
    /* Builds (x, y, z). */
    StaticVec3(f32 x, f32 y, f32 z)
    {
        setVec3(this, x, y, z);
    }
    /* Builds `base` moved `dx` along x. */
    StaticVec3(const Vec3& base, f32 dx)
    {
        setVec3(this, dx + base.x, base.y, base.z);
    }
};

/* The two file-scope vectors the static initialiser (`.ctors` word 0x8056F2D0) builds: the origin, and the
 * origin moved 100 along x. */
static StaticVec3 s_origin(0.0f, 0.0f, 0.0f);
static StaticVec3 s_origin_x100(s_origin, 100.0f);
