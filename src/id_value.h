/*
 * The per-id 4-byte pair the map's tables and item lists use: an id and a signed value/index.  Shared by
 * `fn_80047398.cpp` (`item_count_find`, `item_pair_index_find`, `fn_8004BD30`, `fn_8004C004`),
 * `lobby/fn_801EC9F8.cpp` (the item list and slot table), `menu/menu_item.cpp` (the two pages) and
 * `menu/menu_message.cpp` (the item-list sorters), so it lives in one header (docs/plan.md 6.5 rule 1).
 */
#ifndef MHTRI_ID_VALUE_H
#define MHTRI_ID_VALUE_H

#include "types.h"

/* size: 0x4 */
typedef struct IdValue {
    /* +0x00 */ u16 id;
    /* +0x02 */ s16 value;
} IdValue;

#endif /* MHTRI_ID_VALUE_H */
