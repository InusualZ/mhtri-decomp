/*
 * The camera unit's cross-unit declarations (docs/plan.md 6.5 rule 2).  `camera/fn_802B5C58.cpp`
 * owns these symbols; a consumer declares nothing itself, it includes this header.
 *
 * The four entry points the symbol map already names are C++ free functions with real manglings
 * (`get_camera_pos__Fv`, `set_quake_sub__FUcPQ34nw4r4math4VEC3`), so they are declared at global scope
 * with their real signatures - never as the mangled spelling (rule 9).
 *
 * The band's still-unnamed accessors (`fn_802B9740`, `fn_802B9574`, ...) move here from
 * `include/unsplit/camera.h` as their consumers migrate; that header keeps only what no unit owns.
 */
#ifndef MHTRI_CAMERA_CAMERA_H
#define MHTRI_CAMERA_CAMERA_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* 0x802BDCE0 - the current camera's world position. */
nw4r::math::VEC3 get_camera_pos(void);

/* 0x802BDE14 - the current camera's forward direction. */
nw4r::math::VEC3 get_camera_direction(void);

/* 0x802BDC40 - the current camera's view matrix. */
nw4r::math::MTX34 get_current_view_mtx(void);

/* 0x802BE4B0 - starts a camera quake at `origin`, or at the follow target when `origin` is null. */
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin);

#endif /* __cplusplus */

#endif /* MHTRI_CAMERA_CAMERA_H */
