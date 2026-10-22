/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll`'s `stage_water_surface_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_PL_STAGE_WATER_SURFACE_GET_H
#define MHTRI_PL_STAGE_WATER_SURFACE_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80291BBC (Pl/pl_coll): finds the water cell under `pos`: writes the cell and the surface height, 0 when there is none. */
s32 stage_water_surface_get(nw4r::math::VEC3* pos, u8 area, u16* cell, f32* height, s32 limit);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_STAGE_WATER_SURFACE_GET_H */
