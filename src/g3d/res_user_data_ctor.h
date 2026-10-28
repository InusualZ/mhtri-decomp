/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resanmcamera.cpp`'s `res_user_data_ctor`, the `g3d_resuser_ac.h`
 * `ResUserData(void*)` handle constructor.  C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_USER_DATA_CTOR_H
#define MHTRI_G3D_RES_USER_DATA_CTOR_H

#include "types.h"

struct ResUserData;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008A220 (0x64): stores `value` in the handle and asserts it is 4-byte aligned; returns the handle. */
struct ResUserData* res_user_data_ctor(struct ResUserData* self, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_USER_DATA_CTOR_H */
