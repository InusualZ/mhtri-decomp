/* Leaf header (docs/plan.md 6.5 rule 2): `userdata_item.cpp`'s `userdata_item_total_count`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_USERDATA_ITEM_TOTAL_COUNT_H
#define MHTRI_USERDATA_ITEM_TOTAL_COUNT_H

#include "types.h"

struct LbNpcUserData;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004B624 - counts item `item` over the pouch and box (GetItemData, item_pair_index_find, item_count_find) */
s32 userdata_item_total_count(struct LbNpcUserData* data, u16 item);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_TOTAL_COUNT_H */
