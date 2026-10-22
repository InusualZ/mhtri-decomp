/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s `userdata_item_seen_set`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_USERDATA_ITEM_SEEN_SET_H
#define MHTRI_USERDATA_ITEM_SEEN_SET_H

#include "types.h"

struct LbNpcUserData;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D2B4 - sets bit `item` (<= 746) in the user data's item bit set at +0x4AD8 */
void userdata_item_seen_set(struct LbNpcUserData* data, u16 item);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_SEEN_SET_H */
