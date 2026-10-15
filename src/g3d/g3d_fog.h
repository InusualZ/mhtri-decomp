/* g3d/g3d_fog.h - the declarations `g3d/g3d_fog.cpp` owns (C linkage keeps the map's stems). */
#ifndef MHTRI_G3D_G3D_FOG_H
#define MHTRI_G3D_G3D_FOG_H

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::MTX44 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80079FB4 (0x30): constructs a fog handle at `pSelf` on the record `pData` and returns `pSelf`. */
/* untyped: opaque handle - the FogData record */
s32 fog_ctor(s32 pSelf, void* pData);
/* 0x80079FEC (0xC8): resets the fog the handle at `pSelf` names. */
void fog_init(s32 pSelf);
/* 0x8007A1B0 (0x8): the fog record the handle names (ResCommon::ptr const). */
struct ResHandle;
struct FogData;
const struct FogData* fog_ptr_const(const struct ResHandle* pSelf);
#ifdef __cplusplus
/* 0x8007A2A8 (0xAC): sets the fog's range-adjustment centre and table from `width` and the projection `pProjMtx`. */
void fog_set_range_adj_param(struct ResHandle* pSelf, u16 width, u16 center, const nw4r::math::MTX44* pProjMtx);
#endif

#ifdef __cplusplus
}

struct G3dFog;
struct G3dFogData;

/* 0x8007A0B4 - copies the fog's record into `pDst`. */
extern "C" void fog_copy_to(const G3dFog* pSelf, G3dFogData* pDst);
/* 0x8007A354 - loads the fog into the graphics pipe. */
extern "C" void fog_set_gp(const G3dFog* pSelf);
#endif

#endif /* MHTRI_G3D_G3D_FOG_H */
