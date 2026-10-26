/* Leaf header (docs/plan.md 6.5 rule 2): `lb_interior_fx_spawn`, which `lobby/lb_quest_screen` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_LOBBY_LB_INTERIOR_FX_SPAWN_H
#define MHTRI_LOBBY_LB_INTERIOR_FX_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A7D9C - spawns the interior effects of area `areano` of map `mapno`. */
void lb_interior_fx_spawn(u8 mapno, u8 areano);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_INTERIOR_FX_SPAWN_H */
