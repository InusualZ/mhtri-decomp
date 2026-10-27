/* `lb_shop_open`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SHOP_OPEN_H
#define MHTRI_LOBBY_LB_SHOP_OPEN_H

#include "types.h"

struct _PLW;
struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the NPC shop screen for `plw` and the shop NPC `npc`. */
void lb_shop_open(struct _PLW* plw, struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SHOP_OPEN_H */
