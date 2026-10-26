/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_anmscn.cpp`'s light-set clear. */
#ifndef MHTRI_G3D_OBJ_DELETE_FLAGGED_H
#define MHTRI_G3D_OBJ_DELETE_FLAGGED_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class LightSet;
}  // namespace g3d
}  // namespace nw4r

/* 0x80065284 - fills the light set's light slots with `flag` and returns the set. */
extern "C" nw4r::g3d::LightSet* obj_delete_flagged(nw4r::g3d::LightSet* pSelf, s16 flag);
#endif

#endif /* MHTRI_G3D_OBJ_DELETE_FLAGGED_H */
