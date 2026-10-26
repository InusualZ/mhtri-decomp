/* Leaf header (docs/plan.md 6.5 rule 2): `fog_type_set`, `fog_range_set`, which `g3d/g3d_anmscn` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_G3D_FOG_TYPE_SET_H
#define MHTRI_G3D_FOG_TYPE_SET_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80067AE4 - sets the fog type of the fog handle `fog`. */
void fog_type_set(s32* fog, u32 type);

/* 0x80067A5C - sets the fog start and end of the fog handle `fog`. */
void fog_range_set(s32* fog, f32 start, f32 end);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FOG_TYPE_SET_H */
