#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H

#include "types.h"

/* Leaf header of `lobby/lb_quest_board.cpp`: no record types, so it can sit beside `unsplit/lobby.h` (the owner header
 * spells its own `lobby_w`). */
struct _EFT;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80395C84 - resets the quest board (the first argument is unused). */
void lb_quest_board_reset(s32 unused, s32 mode);
/* 0x80394144 - advances an effect record's state byte; 0x80394154 - retires one pooled effect record. */
void lb_quest_board_state_next(struct _EFT* self);
void lb_quest_board_effect_retire(struct _EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_RESET_H */
