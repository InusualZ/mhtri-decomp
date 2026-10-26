/* Leaf header (docs/plan.md 6.5 rule 2): `sound/se_req.cpp`'s `pl_impact_se_req` (0x800DD898). */
#ifndef MHTRI_SOUND_PL_IMPACT_SE_REQ_H
#define MHTRI_SOUND_PL_IMPACT_SE_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DD898: plays the impact SE at `pos` through the player's sound handle (registry name, GUESS). */
void pl_impact_se_req(struct _PLW* plw, VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_IMPACT_SE_REQ_H */
