/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_screen_step.cpp`'s `lb_screen15_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCREEN15_STEP_H
#define MHTRI_LOBBY_LB_SCREEN15_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80363A5C - the lobby screen step for screen 15 */
void lb_screen15_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN15_STEP_H */
