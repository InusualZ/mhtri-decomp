/* menu/menu_slot_index_is_page_b.h - leaf header: `menu_slot_index_is_page_b` (0x802A9D50), owned by `menu/menu_message.cpp`. */
#ifndef MHTRI_MENU_MENU_SLOT_INDEX_IS_PAGE_B_H
#define MHTRI_MENU_MENU_SLOT_INDEX_IS_PAGE_B_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif
/* Nonzero when slot `index` of the record lies on its second item page. */
s32 menu_slot_index_is_page_b(struct MenuSlot* slot, s16 index);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SLOT_INDEX_IS_PAGE_B_H */
