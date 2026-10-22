/* Leaf header (docs/plan.md 6.5 rule 2): `light/light`'s `light_dir_set`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_LIGHT_LIGHT_DIR_SET_H
#define MHTRI_LIGHT_LIGHT_DIR_SET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C20A4 (light/light): points the scene's first light along `dir`. */
void light_dir_set(nw4r::math::VEC3* dir);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_DIR_SET_H */
