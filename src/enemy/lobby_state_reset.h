/* `lobby_state_reset`, defined by `enemy/em020_prog.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_LOBBY_STATE_RESET_H
#define MHTRI_ENEMY_LOBBY_STATE_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resets the lobby state for entry mode `mode`. */
void lobby_state_reset(u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_STATE_RESET_H */
