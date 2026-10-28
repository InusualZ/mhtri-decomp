/* `quest_group_clear_count`, defined by `menu/menu_quest.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_QUEST_GROUP_CLEAR_COUNT_H
#define MHTRI_MENU_QUEST_GROUP_CLEAR_COUNT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Counts the cleared quests of quest group `group` into `*count`; returns the first uncleared quest id. */
u16 quest_group_clear_count(u16* count, u8 group);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_QUEST_GROUP_CLEAR_COUNT_H */
