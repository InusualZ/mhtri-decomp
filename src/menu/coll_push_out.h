/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_item`'s `coll_push_out`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_MENU_COLL_PUSH_OUT_H
#define MHTRI_MENU_COLL_PUSH_OUT_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80299C14 (menu/menu_item): pushes `pos` out of the collision by `radius`. */
void coll_push_out(nw4r::math::VEC3* pos, f32 radius, s32 kind, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_COLL_PUSH_OUT_H */
