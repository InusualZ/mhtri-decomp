/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em020_prog.cpp` symbol `enemy/em_pop.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_EM020_AREA2_ACTION20_CK_H
#define MHTRI_ENEMY_EM020_AREA2_ACTION20_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8037550C - 1 while the enemy is team 20 in area 2 with its action stack at 30. */
u32 em020_area2_action20_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_AREA2_ACTION20_CK_H */
