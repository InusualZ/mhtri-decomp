/*
 * Declarations for the symbols `src/enemy/em034_prog.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_ENEMY_EM034_PROG_H
#define MHTRI_ENEMY_EM034_PROG_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801B701C */
u32 em034_eft_ck(_ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM034_PROG_H */
