/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_item_page2_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  The map's name. */
#ifndef MHTRI_PL_PL_ITEM_PAGE2_CK_H
#define MHTRI_PL_PL_ITEM_PAGE2_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026FE44 - 1 when the player's +0x2 weapon kind is 4 or 5 */
u32 pl_item_page2_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ITEM_PAGE2_CK_H */
