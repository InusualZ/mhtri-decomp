/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `userdata_server_rows_reset`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_USERDATA_SERVER_ROWS_RESET_H
#define MHTRI_LOBBY_USERDATA_SERVER_ROWS_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C58D4 - resets the user data's server rows (fn_803C5C30) */
void userdata_server_rows_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_USERDATA_SERVER_ROWS_RESET_H */
