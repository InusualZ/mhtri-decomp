/* g3d/g3d_calcworld.h - the cross-unit declarations of `g3d/g3d_calcworld.cpp`, in the consumers' spellings (the
 *   wider form where only a parameter spelling differed). */
#ifndef MHTRI_G3D_G3D_CALCWORLD_H
#define MHTRI_G3D_G3D_CALCWORLD_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void addVec3To(void* dst, const void* src);
void addVec3To(void* dst, const void* src);

/* The world-matrix attribute helpers (0x800737AC..0x800737C4): clear the scale bits for a non-uniform or a
 * uniform-but-not-one scale, or set the uniform / one bits.  Consumers: `src/g3d/fn_800D77B0.cpp` (the per-node
 * transform) and `g3d/g3d_scnmdlsmpl.cpp`. */
u32 world_mtx_attr_not_scale_uniform(u32 attrib);
u32 world_mtx_attr_not_scale_one(u32 attrib);
u32 world_mtx_attr_scale_uniform(u32 attrib);
u32 world_mtx_attr_scale_one(u32 attrib);
/* 0x80074114 - the attribute word of a model's root matrix (every scale bit set). */
u32 world_mtx_attr_root_mtx(void);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ResMdl; } } /* only pointed to here: the full class is g3d/g3d_resmat.h's */

namespace nw4r {
namespace g3d {
class AnmObjChr;
struct FuncObjCalcWorld;
}  // namespace g3d
}  // namespace nw4r

extern "C" {
/* 0x80074074 - the model's `ResMdlInfo` handle (the info block's address). */
u32 res_mdl_get_info(const void* pMdl); /* untyped: opaque handle - the model handle */
/* 0x80074620 - the number of view matrices the model's info block records. */
u32 res_mdl_info_num_view_mtx(const void* pInfo); /* untyped: opaque handle - the info handle */
/* 0x800737CC - runs the model's node-tree byte code: each node's world matrix and attribute word from the base
 * matrix, the character animation and the world callback. */
void g3d_calc_world(nw4r::math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray, const u8* pByteCode,
                    const nw4r::math::MTX34* pBaseMtx, const nw4r::g3d::ResMdl* pMdl,
                    nw4r::g3d::AnmObjChr* pAnmChr, nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 rootAttrib);
/* 0x8007411C - runs the model's node-mix byte code: the skinned matrices blended from the node matrices. */
void g3d_calc_skinning(nw4r::math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray, const nw4r::g3d::ResMdl* pMdl,
                       const u8* pByteCode);
}
#endif

#endif /* MHTRI_G3D_G3D_CALCWORLD_H */
