/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `lb_server_select_screen_step`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SERVER_SELECT_SCREEN_STEP_H
#define MHTRI_LOBBY_LB_SERVER_SELECT_SCREEN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C8960 - the server select screen step (NetCtrlWk::getServerTypeInfo), screen 27 */
void lb_server_select_screen_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_SELECT_SCREEN_STEP_H */
