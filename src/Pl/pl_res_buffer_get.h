/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `pl_res_buffer_get` (0x80267A78), owned by `Pl/player_control.cpp`.
 * GUESS name: the body returns the player's word of `pl_global` +0x44C (NULL before `pl_global` exists), which
 * `nw_resource.cpp`'s player-file loader reads the file into.
 */
#ifndef MHTRI_PL_PL_RES_BUFFER_GET_H
#define MHTRI_PL_PL_RES_BUFFER_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80267A78 (0x28): Returns the player's resource buffer address, or 0. */
u32 pl_res_buffer_get(u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_RES_BUFFER_GET_H */
