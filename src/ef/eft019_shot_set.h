/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft019.cpp`'s `eft019_shot_set` (0x80112B7C). */
#ifndef MHTRI_EF_EFT019_SHOT_SET_H
#define MHTRI_EF_EFT019_SHOT_SET_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;
struct _CP_VECTOR;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80112B7C: spawns an eft019 effect of `id` at `pos` with the rotation `rot` and the scale `scale`; id 0x66
 * takes its colour from the player's blade record (registry name, GUESS). */
void eft019_shot_set(struct _PLW* plw, u8 id, u8 colour, VEC3* pos, struct _CP_VECTOR* rot, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT019_SHOT_SET_H */
