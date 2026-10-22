/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `system_lobby_flag_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_EF_SYSTEM_LOBBY_FLAG_GET_H
#define MHTRI_EF_SYSTEM_LOBBY_FLAG_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0724 - returns system_w byte +0x877; lb_npc gates the master player's lobby frame on it */
u8 system_lobby_flag_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_LOBBY_FLAG_GET_H */
