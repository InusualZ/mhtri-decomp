/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_scnobj.cpp`'s `scn_obj_callback_dtor` (the scene-object callback
 * base's destructor), for a consumer whose callback is laid out without the class.  C linkage (the map row is a
 * plain name). */
#ifndef MHTRI_G3D_SCN_OBJ_CALLBACK_DTOR_H
#define MHTRI_G3D_SCN_OBJ_CALLBACK_DTOR_H

#include "types.h"

struct LbNpcMatCallback;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008147C - destroys the callback base of `self`; frees it when `flag` is positive. */
struct LbNpcMatCallback* scn_obj_callback_dtor(struct LbNpcMatCallback* self, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_SCN_OBJ_CALLBACK_DTOR_H */
