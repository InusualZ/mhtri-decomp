/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `pl_res_loaded_set` (0x802673D4), owned by `Pl/player_control.cpp`.
 * GUESS name: the body stores 1 into the player's byte of the `.sbss` table 0x80794B2C; `nw_resource.cpp`'s
 * player-file load callback calls it once the file is registered.
 */
#ifndef MHTRI_PL_PL_RES_LOADED_SET_H
#define MHTRI_PL_PL_RES_LOADED_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802673D4 (0x14): Marks the player's resource file as loaded. */
void pl_res_loaded_set(u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_RES_LOADED_SET_H */
