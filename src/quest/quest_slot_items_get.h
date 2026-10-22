/* `quest_slot_items_get`, defined by `quest/quest_entry.cpp` (leaf header, rule 2). */
#ifndef MHTRI_QUEST_QUEST_SLOT_ITEMS_GET_H
#define MHTRI_QUEST_QUEST_SLOT_ITEMS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the quest ids of board slot `index` into `out`. */
void quest_slot_items_get(u8 index, u16* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_SLOT_ITEMS_GET_H */
