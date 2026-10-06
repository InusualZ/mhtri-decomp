/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em019_ai.cpp` symbol `menu/multi_result.cpp` calls.  C linkage (the
 * map row is a plain name). */
#ifndef MHTRI_ENEMY_EM019_ACTION13_ACTIVE_CK_H
#define MHTRI_ENEMY_EM019_ACTION13_ACTIVE_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80382F94 - 1 while the record is in action 13 with a sub-state of 2..8 (GUESS name). */
u32 em019_action13_active_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM019_ACTION13_ACTIVE_CK_H */
