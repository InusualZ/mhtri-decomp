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

struct _ENEMY_WORK;
struct _PLW;

/* 0x802BDCE0 - the current camera's world position. */
nw4r::math::VEC3 get_camera_pos(void);

/* 0x802BDE14 - the current camera's forward direction. */
nw4r::math::VEC3 get_camera_direction(void);

/* 0x802BDC40 - the current camera's view matrix. */
nw4r::math::MTX34 get_current_view_mtx(void);

/* 0x802BE4B0 - starts a camera quake at `origin`, or at the follow target when `origin` is null. */
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin);

/* 0x802BC564 - the per-id light-record handler the bank queries fall through to (its consumer is
 * `light/light.cpp`, which needed these three declared where their owner is).  `extern "C"` because
 * the owner defines all three inside its file-wide `extern "C"` block. */
extern "C" {
void fn_802BC564(u8 id, void* arg);

/* 0x802BE3EC - whether the camera work's +0x284 byte holds 1. */
bool fn_802BE3EC(void);

/* 0x802BE7E8 - updates one of a light work's channels.  The parameters are typed `void*` because
 * the light work's own types are private to its unit. */
void fn_802BE7E8(void* self, void* channel, u8 index);

/* The small camera-band setters and predicates 0x802B8DF8/0x802BBA64/0x802BBAC0/0x802BBAC4/
 * 0x802BE638 - the rest of the `.text` range this unit owns (0x802B5C58-0x802BEAAC).  Their
 * consumers read them out of `include/unsplit/lobby.h` and `include/unsplit/unknown.h` while the
 * addresses were unclaimed; both of those headers include this one now (docs/plan.md 6.5 rule 2).
 * `fn_802BE638`/`fn_802B8DF8` keep the call sites' spellings - neither body is written yet. */

/* 0x802BBA64 - stores the argument in the camera work's +0x47E byte (1 while the sub-scene is up). */
void fn_802BBA64(u8 value);

/* 0x802BBAC0 - the four-byte stub at 0x802BBAC0. */
void fn_802BBAC0(void);

/* 0x802BBAC4 - like `fn_802BBA64`, for the +0x47F byte. */
void fn_802BBAC4(u8 value);

/* 0x802B8DF8 - r3 the player work; `Pl/fn_802489D4.cpp`'s leaves call it. */
void fn_802B8DF8(struct _PLW* self);

/* 0x802BE638 - r3 the enemy work, r4 a kind, r5 an output `Vec3`; `enemy/fn_801A4504.cpp` calls it. */
void fn_802BE638(struct _ENEMY_WORK* self, s32 a, Vec3* v);
}

#endif /* __cplusplus */

#endif /* MHTRI_CAMERA_CAMERA_H */
