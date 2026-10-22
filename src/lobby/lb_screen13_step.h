/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp`'s `lb_screen13_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCREEN13_STEP_H
#define MHTRI_LOBBY_LB_SCREEN13_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033C9B0 - the lobby screen step for screen 13 */
void lb_screen13_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN13_STEP_H */
