/* Leaf header (docs/plan.md 6.5 rule 2): `fog_set_color`, which `g3d/g3d_anmscn` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_G3D_FOG_SET_COLOR_H
#define MHTRI_G3D_FOG_SET_COLOR_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800679D4 - sets the fog colour of the fog handle `fog`. */
void fog_set_color(s32* fog, GXColor* colour);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FOG_SET_COLOR_H */
