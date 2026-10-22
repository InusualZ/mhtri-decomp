/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_prog_support.cpp`'s `lb_screen16_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_ENEMY_LB_SCREEN16_STEP_H
#define MHTRI_ENEMY_LB_SCREEN16_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803831B4 - the lobby screen step for screen 16 */
void lb_screen16_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LB_SCREEN16_STEP_H */
