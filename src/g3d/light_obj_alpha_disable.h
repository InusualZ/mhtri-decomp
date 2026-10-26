/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_anmscn.cpp`'s light-object alpha switch. */
#ifndef MHTRI_G3D_LIGHT_OBJ_ALPHA_DISABLE_H
#define MHTRI_G3D_LIGHT_OBJ_ALPHA_DISABLE_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class LightObj;
}  // namespace g3d
}  // namespace nw4r

/* 0x80064BE4 - sets the light object's alpha-disable flag (bit 0x20 of LightObj::mFlag). */
extern "C" void light_obj_alpha_disable(nw4r::g3d::LightObj* p);
#endif

#endif /* MHTRI_G3D_LIGHT_OBJ_ALPHA_DISABLE_H */
