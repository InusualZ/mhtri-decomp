/* Leaf header (docs/plan.md 6.5 rule 2): `userdata_item.cpp`'s `userdata_equip_item_id_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_USERDATA_EQUIP_ITEM_ID_GET_H
#define MHTRI_USERDATA_EQUIP_ITEM_ID_GET_H

#include "types.h"

struct LbNpcUserData;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004AFC8 - returns the item id of equip-pouch slot `index` (lhzx of userdata_equip_item_slots_get()[index]) */
u16 userdata_equip_item_id_get(struct LbNpcUserData* data, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_EQUIP_ITEM_ID_GET_H */
