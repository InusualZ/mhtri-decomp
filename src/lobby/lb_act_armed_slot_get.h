/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s `lb_act_armed_slot_get`, the one declaration
 * `menu/menu_plsearch.cpp` needs. */
#ifndef MHTRI_LOBBY_LB_ACT_ARMED_SLOT_GET_H
#define MHTRI_LOBBY_LB_ACT_ARMED_SLOT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8020D624 (0xBC): the slot of `lb_slot_act_tbl` whose act the player is running (act family 11 only), or 0 */
s32 lb_act_armed_slot_get(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ACT_ARMED_SLOT_GET_H */
