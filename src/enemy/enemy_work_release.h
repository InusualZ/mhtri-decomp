/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/enemy_control.cpp`'s `enemy_work_release`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_ENEMY_ENEMY_WORK_RELEASE_H
#define MHTRI_ENEMY_ENEMY_WORK_RELEASE_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80143190 (0x1F4): releases one enemy work. */
void enemy_work_release(struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_ENEMY_WORK_RELEASE_H */
