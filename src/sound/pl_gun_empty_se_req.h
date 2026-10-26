/* Leaf header (docs/plan.md 6.5 rule 2): `sound/se_req.cpp`'s `pl_gun_empty_se_req`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_SOUND_PL_GUN_EMPTY_SE_REQ_H
#define MHTRI_SOUND_PL_GUN_EMPTY_SE_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

extern "C" {

/* 0x800DDC04 - plays the empty-magazine sound (se 20) at `pos` through the actor's `_se_w` (registry name). */
void pl_gun_empty_se_req(struct _PLW* self, nw4r::math::VEC3* pos);

}

#endif /* MHTRI_SOUND_PL_GUN_EMPTY_SE_REQ_H */
