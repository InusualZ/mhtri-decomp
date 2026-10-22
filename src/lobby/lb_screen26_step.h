/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `lb_screen26_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCREEN26_STEP_H
#define MHTRI_LOBBY_LB_SCREEN26_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C59A0 - the lobby screen step for screen 26 (eft052_item_get_open) */
void lb_screen26_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN26_STEP_H */
