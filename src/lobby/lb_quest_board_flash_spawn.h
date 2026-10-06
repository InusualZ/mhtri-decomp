/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_quest_board.cpp` row the Poogie screen calls.  C linkage. */
#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_FLASH_SPAWN_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_FLASH_SPAWN_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80395F18 - finds the NPC's board object (kind 8) and stores its three parameters (GUESS name). */
void lb_quest_board_flash_spawn(struct _LB_NPC* npc, s16 a, s16 b, s16 c);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_FLASH_SPAWN_H */
