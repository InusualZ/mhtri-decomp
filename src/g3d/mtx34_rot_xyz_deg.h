/* `mtx34_rot_xyz_deg`, defined by `g3d/g3d_resanmchr.cpp` (leaf header, rule 2). */
#ifndef MHTRI_G3D_MTX34_ROT_XYZ_DEG_H
#define MHTRI_G3D_MTX34_ROT_XYZ_DEG_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sets `mtx` to the rotation by the three angles in degrees (nw4r's `MTX34RotXYZDeg`, out of line). */
void mtx34_rot_xyz_deg(nw4r::math::MTX34* mtx, f32 x, f32 y, f32 z);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_MTX34_ROT_XYZ_DEG_H */
