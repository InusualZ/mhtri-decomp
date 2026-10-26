/* `item_slots_room_get`, defined by `userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_ITEM_SLOTS_ROOM_GET_H
#define MHTRI_USERDATA_ITEM_ITEM_SLOTS_ROOM_GET_H

#include "types.h"

struct IdValue;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004B7B0 - how many more of item `id` the `count` slots of `slots` can take. */
s32 item_slots_room_get(u16 id, struct IdValue* slots, u16 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_ITEM_SLOTS_ROOM_GET_H */
