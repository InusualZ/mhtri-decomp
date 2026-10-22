/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em020_prog` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_QUEST_INFO_PAGE_OPEN_H
#define MHTRI_ENEMY_QUEST_INFO_PAGE_OPEN_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8036DCD0 - opens the quest info page: sets the slot's page kind to 10 */
void quest_info_page_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_QUEST_INFO_PAGE_OPEN_H */
