/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_hit_result_best_get`, which `Pl/pl_act_step.cpp` calls. */
#ifndef MHTRI_PL_PL_HIT_RESULT_BEST_GET_H
#define MHTRI_PL_PL_HIT_RESULT_BEST_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027CB1C - the larger hit result of the actor's two attack entries, 255 for none (registry name). */
u8 pl_hit_result_best_get(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_HIT_RESULT_BEST_GET_H */
