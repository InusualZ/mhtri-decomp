/* camera/camera_quake_pos_req.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `camera_quake_pos_req` (0x802BE6AC), which `camera/camera_main.cpp` owns;
 * it requests a camera quake at `pos` when the enemy is in the player's area. */
#ifndef MHTRI_CAMERA_CAMERA_QUAKE_POS_REQ_H
#define MHTRI_CAMERA_CAMERA_QUAKE_POS_REQ_H

#include "types.h"

struct _ENEMY_WORK;
#ifdef __cplusplus
namespace nw4r { namespace math { struct VEC3; } }
#endif

#ifdef __cplusplus
extern "C" {
#endif

void camera_quake_pos_req(struct _ENEMY_WORK* self, u8 kind, nw4r::math::VEC3* pos, u8 level);

#ifdef __cplusplus
}
#endif

#endif
