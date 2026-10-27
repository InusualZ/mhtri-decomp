/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s `pl_player_count_get`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_PL_PL_PLAYER_COUNT_GET_H
#define MHTRI_PL_PL_PLAYER_COUNT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802673B8 (0x1C): the player count `pl_global` holds (0 without it). GUESS name. */
s32 pl_player_count_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PLAYER_COUNT_GET_H */
