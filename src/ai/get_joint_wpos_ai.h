/* `get_joint_wpos_ai`, defined by `ai/ai_npc.cpp` (leaf header, rule 2). C++ linkage: the map's row is the mangling
 * `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`. */
#ifndef MHTRI_AI_GET_JOINT_WPOS_AI_H
#define MHTRI_AI_GET_JOINT_WPOS_AI_H

#include "types.h"
#include "nw4r/math.h"

struct _AINPC_W;

/* Reads joint `joint`'s world position of the NPC's embedded model into `out`. */
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out);

#endif /* MHTRI_AI_GET_JOINT_WPOS_AI_H */
