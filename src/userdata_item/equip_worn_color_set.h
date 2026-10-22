/* `equip_worn_color_set`, defined by `src/userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_EQUIP_WORN_COLOR_SET_H
#define MHTRI_USERDATA_ITEM_EQUIP_WORN_COLOR_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dyes worn armour piece `part` (its default colour when `use_default`) in the snapshot and the box record. */
/* untyped: the save block, seen by each caller through its own record view */
void equip_worn_color_set(void* userdata, u8 part, u16 color, u8 use_default);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_EQUIP_WORN_COLOR_SET_H */
