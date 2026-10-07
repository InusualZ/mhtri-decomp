/* enemy/em015_damage_level.h - leaf header (docs/plan.md 6.5 rule 2) for `em015_damage_level`, which
 *   `enemy/em015_prog.cpp` owns; `ef/eft013_fx.cpp` reads the level to pick its glow effects. */
#ifndef MHTRI_ENEMY_EM015_DAMAGE_LEVEL_H
#define MHTRI_ENEMY_EM015_DAMAGE_LEVEL_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80182918 - the damage counter's level (0 below 1, 1 below 100, then from the part-3 damage level). */
u32 em015_damage_level(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM015_DAMAGE_LEVEL_H */
