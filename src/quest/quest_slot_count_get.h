/* `quest_slot_count_get`, defined by `quest/quest_entry.cpp` (leaf header, rule 2). */
#ifndef MHTRI_QUEST_QUEST_SLOT_COUNT_GET_H
#define MHTRI_QUEST_QUEST_SLOT_COUNT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the number of quests in board slot `index`. */
s32 quest_slot_count_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_SLOT_COUNT_GET_H */
