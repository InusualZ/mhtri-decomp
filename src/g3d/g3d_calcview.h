/* g3d/g3d_calcview.h - the `ResMdl` handle validity accessors `g3d/g3d_calcview.cpp` owns (C linkage). */
#ifndef MHTRI_G3D_G3D_CALCVIEW_H
#define MHTRI_G3D_G3D_CALCVIEW_H

#include "types.h"
#include "nw4r/math.h"

#include "g3d/res_mdl_info.h" /* ResMdlInfoData, what res_mdl_info_data returns (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

const char* res_mdl_info_get_class_name(void);      /* 0x8006FFBC - the `ResMdlInfo` type name the assert prints */
u32 res_mdl_info_is_valid(void* pSelf);       /* 0x8006FFC8 - the `ResMdlInfo` handle validity test */

/* The checked resource resolver `g3d/g3d_resfile.cpp` calls. */

/* 0x800710BC - `out = a * b` for two 3x4 matrices. */
void mtx34_concat(Mtx34* out, const Mtx34* a, const Mtx34* b);

/* 0x8006FDCC..0x8007100C - the `g3d_calcworld` node/resource helpers (callers: g3d_calcworld.cpp and the
 * matrix users).  `fn_8006FDCC`'s callers use the word as a matrix id or an array index. */
u32 fn_8006FDCC(const void* p);
/* untyped: opaque handle - the ResMdlInfo handle */
const struct ResMdlInfoData* res_mdl_info_data(const void* pInfo); /* 0x8006FF50 - the info block, asserting the handle is valid */
/* 0x8006FEC8 - the node id of position/normal matrix `mtxID` */
u32 res_mdl_info_get_node_of_pos_nrm_mtx(const void* pInfo, u32 mtxID); /* untyped: opaque handle - the info handle */
s32* fn_80070054(void* pOut, const void* pKey);
void mtx34_copy_ps(Mtx34* pDst, const Mtx34* pSrc);   /* 0x8007100C - paired-single copy of a 3x4 matrix */

/* 0x8006FFDC - the number of position/normal matrices the model's info block records. */
u32 res_mdl_info_num_pos_nrm_mtx(const void* pInfo); /* untyped: opaque handle - the info handle */
/* 0x80071C38 / 0x80071C3C / 0x80071C40 - the locked-cache helpers: wait for the DMA queue, invalidate a block's
 * cache lines, and the locked cache's base address. */
void g3d_lc_queue_wait(u32 length);
void g3d_dc_invalidate_range(void* pStart, u32 size); /* untyped: byte range */
void* g3d_lc_base(void); /* untyped: byte range - the locked cache */

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ResMdl; } } /* only pointed to here: the full class is g3d/g3d_resmat.h's */

extern "C" {
/* 0x80070820 / 0x80071198 / 0x80071C48 - the model's view matrices (position, normal, texture) from its world
 * matrices and the camera: in main memory, in the locked cache, and in the locked cache with the world matrices
 * brought in by DMA. */
void g3d_calc_view(nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
                   const nw4r::math::MTX34* pWorldMtxArray, const u32* pWorldMtxAttribArray, u32 numMtx,
                   const nw4r::math::MTX34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                   nw4r::math::MTX34* pViewTexMtxArray);
void g3d_calc_view_lc(nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
                      const nw4r::math::MTX34* pWorldMtxArray, const u32* pWorldMtxAttribArray, u32 numMtx,
                      const nw4r::math::MTX34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                      nw4r::math::MTX34* pViewTexMtxArray);
void g3d_calc_view_lc_dma(nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
                          const nw4r::math::MTX34* pWorldMtxArray, const u32* pWorldMtxAttribArray, u32 numMtx,
                          const nw4r::math::MTX34* pCamera, const nw4r::g3d::ResMdl* pMdl,
                          nw4r::math::MTX34* pViewTexMtxArray);
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVIEW_H */
