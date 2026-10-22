/* Leaf header (docs/plan.md 6.5 rule 2): symbols `enemy/em_common.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_ENEMY_EM_SIGHT_PLAYER_CK2_H
#define MHTRI_ENEMY_EM_SIGHT_PLAYER_CK2_H

#include "types.h"
#include "enemy/ENEMY_WORK.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80131910: whether the enemy sees player `player`. */
u32 em_sight_player_ck2(_ENEMY_WORK* enemy, u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_SIGHT_PLAYER_CK2_H */
