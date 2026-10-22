/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E0ADC.cpp`'s `lb_screen03_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCREEN03_STEP_H
#define MHTRI_LOBBY_LB_SCREEN03_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E5E98 - the lobby screen step for screen 3 (lobby_w.state 3 in the lobby screen dispatch) */
void lb_screen03_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN03_STEP_H */
