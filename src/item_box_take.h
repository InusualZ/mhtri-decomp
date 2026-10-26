/* Leaf header (docs/plan.md 6.5 rule 2): symbols `userdata_item.cpp` defines, for a consumer that does not include
 * the owner's source.  C linkage (the map rows are plain names). */
#ifndef MHTRI_ITEM_BOX_TAKE_H
#define MHTRI_ITEM_BOX_TAKE_H

#include "types.h"
#include "id_value.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004B870 - adds `delta` to the count of the item box's slot `index`. */
void item_box_slot_take(IdValue* box, u16 index, s16 delta);
/* 0x8004B8BC - adds `delta` to item `id` in the item box; returns what could not be taken. */
s16 item_box_take(u16 id, s16 delta);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ITEM_BOX_TAKE_H */
