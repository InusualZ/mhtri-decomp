/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `pl_common_motion_res_ids`, which `Pl/player_control.cpp`
 * reads.  GUESS name (the registry carries it). */
#ifndef MHTRI_PL_COMMON_MOTION_RES_IDS_H
#define MHTRI_PL_COMMON_MOTION_RES_IDS_H

#include "types.h"
#include "lobby/lb_npc.h"   /* LbResId */

/* 0x80582B30 (0x28): the players' common motion bank names, one 8-byte record per common motion set. */
extern LbResId pl_common_motion_res_ids[];

#endif /* MHTRI_PL_COMMON_MOTION_RES_IDS_H */
