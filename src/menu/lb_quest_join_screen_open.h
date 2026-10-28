/* `lb_quest_join_screen_open`, defined by `menu/menu_quest.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_LB_QUEST_JOIN_SCREEN_OPEN_H
#define MHTRI_MENU_LB_QUEST_JOIN_SCREEN_OPEN_H

#include "types.h"

struct LbQuestBoardData;
struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the quest join screen on `board` for the NPC `npc`. */
void lb_quest_join_screen_open(struct LbQuestBoardData* board, struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_QUEST_JOIN_SCREEN_OPEN_H */
