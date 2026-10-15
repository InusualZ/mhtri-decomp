/* g3d/g3d_fog.h - the declarations `g3d/g3d_fog.cpp` owns (C linkage keeps the map's stems). */
#ifndef MHTRI_G3D_G3D_FOG_H
#define MHTRI_G3D_G3D_FOG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80079FB4 (0x30): constructs a fog handle at `pSelf` on the record `pData` and returns `pSelf`. */
/* untyped: opaque handle - the FogData record */
s32 fog_ctor(s32 pSelf, void* pData);
/* 0x80079FEC (0xC8): resets the fog the handle at `pSelf` names. */
void fog_init(s32 pSelf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_FOG_H */
