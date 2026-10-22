/* `quest_board_flags_send`, defined by `enemy/em020_prog.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_QUEST_BOARD_FLAGS_SEND_H
#define MHTRI_ENEMY_QUEST_BOARD_FLAGS_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sends the player's quest board flags and objective kind to the session. */
void quest_board_flags_send(u8 flags, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_QUEST_BOARD_FLAGS_SEND_H */
