/* Leaf header (docs/plan.md 6.5 rule 2): the `quest/quest_entry.cpp` symbols `lobby/lb_quest_screen.cpp` calls, for a
 * consumer that cannot include the owner's full header (`quest/quest_entry.h` reaches `pl.h`, whose `MHchar` and
 * `_GXChannelID` clash with `sound/mhchar.h`'s, which the note pane's `NoteWork` needs).  C linkage, as in the owner. */
#ifndef MHTRI_QUEST_QUEST_RECORD_FIND_H
#define MHTRI_QUEST_QUEST_RECORD_FIND_H

#include "types.h"
#include "quest/quest_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The result record of quest `quest_id` from the loaded quest list, or NULL when none carries that id. */
QuestRecord* quest_record_find(u16 quest_id);
/* The index of the loaded element a quest id names (0x803ADF84), or -1. */
s32 quest_element_find(u8 kind);
/* The record's +0x372 word, or the current record's (0x803B427C). */
u16 quest_field372_get(QuestRecord* rec);
/* Applies `used` of the arena item `id` to the work block's item table (0x803B1DB8). */
void quest_element_item_apply(u8 id, u16 count, u16 used);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_RECORD_FIND_H */
