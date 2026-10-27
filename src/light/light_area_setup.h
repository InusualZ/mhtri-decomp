/* Leaf header (docs/plan.md 6.5 rule 2): `light_obj_area_select`, `light_obj_init`, `light_area_setup`, which `light/light` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_LIGHT_LIGHT_AREA_SETUP_H
#define MHTRI_LIGHT_LIGHT_AREA_SETUP_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#ifdef __cplusplus
namespace nw4r { namespace g3d { struct LightObj; } }

extern "C" {

/* 0x802C1D68 - returns light object `index` of the scene. */
nw4r::g3d::LightObj* light_obj_area_select(u8 index);

/* 0x802C1EA0 - initialises light object `obj` at `pos` with `colour`. */
void light_obj_init(nw4r::g3d::LightObj* obj, nw4r::math::VEC3* pos, GXColor colour, s32 mode);

/* 0x802C20DC - seats the current area's light objects. */
void light_area_setup(void);

}
#endif

#endif /* MHTRI_LIGHT_LIGHT_AREA_SETUP_H */
