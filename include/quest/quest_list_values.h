/*
 * Leaf header (docs/plan.md 6.5 rule 2): `quest_list_values`, the u16 key/value array `quest/quest_entry.cpp`
 * owns (`.sbss` 0x80794C24, the third word of its list-block run) and `quest/quest_entry.cpp` reads.  It is
 * separate from `quest/quest_entry.h` so a reader does not take that header's record types with it.
 */
#ifndef MHTRI_QUEST_QUEST_LIST_VALUES_H
#define MHTRI_QUEST_QUEST_LIST_VALUES_H

#include "types.h"

/* The array inside the quest list block `quest_list_load_hunt`/`_arena` allocate (block + 0x1A0). */
extern u16* quest_list_values;

#endif /* MHTRI_QUEST_QUEST_LIST_VALUES_H */
