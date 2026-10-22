/* enemy/em025_prog.h - the declarations of `enemy/em025_prog.cpp` its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_ENEMY_EM025_PROG_H
#define MHTRI_ENEMY_EM025_PROG_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8019E70C - the shore action's sub-phase of enemy record `self` (255 when it is not that action). */
u32 em025_shore_phase_get(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM025_PROG_H */
