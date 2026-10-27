/* `lb_screen13_open`, defined by `lobby/lb_companion_ui.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SCREEN13_OPEN_H
#define MHTRI_LOBBY_LB_SCREEN13_OPEN_H

#include "types.h"

struct _PLW;
struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Opens lobby screen 13 for `player` and the NPC `npc`. */
void lb_screen13_open(struct _PLW* player, struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN13_OPEN_H */
