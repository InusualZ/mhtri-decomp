/* enemy/em003_prog.h - the declarations of `enemy/em003_prog.cpp` its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_ENEMY_EM003_PROG_H
#define MHTRI_ENEMY_EM003_PROG_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8015D6A8 - the em003 state test `ef/eft_slot.cpp`'s area rating skips an enemy on (1 = skip). */
u8 em003_skip_ck(struct _ENEMY_WORK* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM003_PROG_H */
