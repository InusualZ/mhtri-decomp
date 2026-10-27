/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_combo` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_SLOT_WORKER_BUSY_CK_H
#define MHTRI_MENU_MENU_SLOT_WORKER_BUSY_CK_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031AC04 - tail-calls the Pl predicate on the slot's player work (+0x190): whether its +0x30A byte is set; gates two top-menu rows */
s32 menu_slot_worker_busy_ck(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SLOT_WORKER_BUSY_CK_H */
