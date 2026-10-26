/* `userdata_box_capacity`, defined by `userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_USERDATA_BOX_CAPACITY_H
#define MHTRI_USERDATA_ITEM_USERDATA_BOX_CAPACITY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004AE70 - the number of equipment-box records the user data `userdata` has room for. */
u16 userdata_box_capacity(u8* userdata);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_USERDATA_BOX_CAPACITY_H */
