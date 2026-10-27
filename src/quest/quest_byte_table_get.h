/* Leaf header (docs/plan.md 6.5 rule 2): `quest/quest_entry.cpp`'s `quest_byte_table_get`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_QUEST_QUEST_BYTE_TABLE_GET_H
#define MHTRI_QUEST_QUEST_BYTE_TABLE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803B3440 (0x14): the quest's spawn table byte for `map`. */
u8 quest_byte_table_get(u8 map);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_BYTE_TABLE_GET_H */
