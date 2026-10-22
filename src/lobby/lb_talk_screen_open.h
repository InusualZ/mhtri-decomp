/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_talk_screen_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_TALK_SCREEN_OPEN_H
#define MHTRI_LOBBY_LB_TALK_SCREEN_OPEN_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801EC680 - clears the lobby screen work and opens talk screen `kind` for player `self` (memset lobby_w, sysSE_req) */
void lb_talk_screen_open(struct _PLW* self, u32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TALK_SCREEN_OPEN_H */
