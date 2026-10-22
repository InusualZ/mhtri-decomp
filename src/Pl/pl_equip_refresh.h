/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_equip_refresh`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_EQUIP_REFRESH_H
#define MHTRI_PL_PL_EQUIP_REFRESH_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80272A08 - refreshes the player's equipment records (equip_record_copy) */
void pl_equip_refresh(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_EQUIP_REFRESH_H */
