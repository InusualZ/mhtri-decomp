/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_802FA9A0.cpp`'s `lb_unlock_rank_get`, which `lobby/lb_menu_pos_tbl.cpp` calls. */
#ifndef MHTRI_LOBBY_LB_UNLOCK_RANK_GET_H
#define MHTRI_LOBBY_LB_UNLOCK_RANK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802FB840 (0x84): the village rank the unlock table has reached (0, or 1..4); a halfword, which the callers
 * zero-extend with `clrlwi 16` (`lobby/lb_npc.cpp` spells it the same). */
u16 lb_unlock_rank_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_UNLOCK_RANK_GET_H */
