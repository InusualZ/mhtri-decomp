/* ef/eft022_spawn.h - the declaration of `eft022_spawn`, which `ef/eft022_fx.cpp` owns (docs/plan.md 6.5 rule 2),
 * with the callers' `_ENEMY_WORK*` view of the source (the same pointer register). */
#ifndef MHTRI_EF_EFT022_SPAWN_H
#define MHTRI_EF_EFT022_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80116430 - creates the effect's eft022 record for an actor at `pos` when `area` is the current area. */
void eft022_spawn(struct _ENEMY_WORK* source, nw4r::math::VEC3* pos, u8 area);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT022_SPAWN_H */
