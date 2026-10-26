/* menu/menu_slot_find.h - leaf header: `menu_slot_find` (0x802A9E20), owned by `menu/menu_message.cpp`. */
#ifndef MHTRI_MENU_MENU_SLOT_FIND_H
#define MHTRI_MENU_MENU_SLOT_FIND_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif
/* Returns the slot index of item `id` in the record's pages, 0xFF when none holds it. */
u8 menu_slot_find(struct MenuSlot* slot, u16 id);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SLOT_FIND_H */
