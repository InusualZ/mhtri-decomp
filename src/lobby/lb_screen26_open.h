/* `lb_screen26_open`, defined by `lobby/lb_server_sel_trans.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SCREEN26_OPEN_H
#define MHTRI_LOBBY_LB_SCREEN26_OPEN_H

#include "types.h"

struct _PLW;
struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Opens lobby screen 26 for `player` and the NPC `npc`. */
void lb_screen26_open(struct _PLW* player, struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN26_OPEN_H */
