/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft004_fx.cpp`'s `eft004_hold_item_spawn`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_EF_EFT004_HOLD_ITEM_SPAWN_H
#define MHTRI_EF_EFT004_HOLD_ITEM_SPAWN_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 0x80101594 - spawns the held-item effect for player `self` from effect resource slot 12 */
/* untyped: an opaque handle passed through - the player actor, as the owner declares it */
void eft004_hold_item_spawn(void* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_HOLD_ITEM_SPAWN_H */
