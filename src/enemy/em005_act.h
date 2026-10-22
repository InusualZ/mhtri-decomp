/* enemy/em005_act.h - the declarations `enemy/em005_act.cpp` owns that other units call. */
#ifndef MHTRI_ENEMY_EM005_ACT_H
#define MHTRI_ENEMY_EM005_ACT_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* Answers 1 while the record runs one of the first five steps of action 13 (`enemy/em030_prog.cpp` calls it). */
u32 em005_act13_early_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
