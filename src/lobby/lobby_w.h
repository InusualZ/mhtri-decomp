/*
 * `lobby_w` - the one declaration of the lobby work block (rule 2), defined by `src/lobby/lb_menu_pos_tbl.cpp`
 * (its `.bss` 0x806AAA88-0x806AACC0).  The record type `LbLobbyWork` lives in `lobby/lobby_work.h` (rule 1).
 */
#ifndef MHTRI_LOBBY_LOBBY_W_H
#define MHTRI_LOBBY_LOBBY_W_H

#include "lobby/lobby_work.h"

#ifdef __cplusplus
extern "C" {
#endif

extern LbLobbyWork lobby_w;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LOBBY_W_H */
