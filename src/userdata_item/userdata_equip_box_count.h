/* `userdata_equip_box_count`, defined by `userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_USERDATA_EQUIP_BOX_COUNT_H
#define MHTRI_USERDATA_ITEM_USERDATA_EQUIP_BOX_COUNT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004C368 - the number of equipment-box records of kind `kind` and id `id`. */
u16 userdata_equip_box_count(u8 kind, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_USERDATA_EQUIP_BOX_COUNT_H */
