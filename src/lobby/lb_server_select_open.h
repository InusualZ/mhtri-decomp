/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `lb_server_select_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SERVER_SELECT_OPEN_H
#define MHTRI_LOBBY_LB_SERVER_SELECT_OPEN_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C88E4 - clears the lobby screen work and opens the server select for player `self` */
void lb_server_select_open(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SERVER_SELECT_OPEN_H */
