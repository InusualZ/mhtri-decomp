/* enemy/em018_glow_get.h - leaf header for `em018_glow_get`, which `src/enemy/em018_prog.cpp` defines with C
 * linkage over its own work view (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_ENEMY_EM018_GLOW_GET_H
#define MHTRI_ENEMY_EM018_GLOW_GET_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80192440 - the enemy's glow level (the f32 at +0x34C).  GUESS name. */
f32 em018_glow_get(struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM018_GLOW_GET_H */
