/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `pl_weapon_motion_res_ids`, which `Pl/pl_motion.cpp` reads. */
#ifndef MHTRI_PL_WEAPON_MOTION_RES_IDS_H
#define MHTRI_PL_WEAPON_MOTION_RES_IDS_H

#include "types.h"
#include "lobby/lb_npc.h"   /* LbResId */

/* 0x80582C10 (0x50): the players' weapon motion bank names, one 8-byte record per weapon class. GUESS name. */
extern LbResId pl_weapon_motion_res_ids[];

#endif /* MHTRI_PL_WEAPON_MOTION_RES_IDS_H */
