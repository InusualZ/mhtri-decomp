#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H

#include "types.h"

/* Leaf header of `lobby/lb_quest_board.cpp`: no record types, so it can sit beside `unsplit/lobby.h` (the owner header
 * spells its own `lobby_w`). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80395C84 - resets the quest board (the first argument is unused). */
void lb_quest_board_reset(s32 unused, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H */
