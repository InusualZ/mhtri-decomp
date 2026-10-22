/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_common`'s `em_kill_cut_target_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_ENEMY_EM_KILL_CUT_TARGET_GET_H
#define MHTRI_ENEMY_EM_KILL_CUT_TARGET_GET_H

#include "types.h"

#ifdef __cplusplus
struct _ENEMY_WORK;
struct _PLW;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8012D498 (enemy/em_common): the monster the player's finishing blow targets, or null. */
struct _ENEMY_WORK* em_kill_cut_target_get(s32 kind, struct _PLW* player, s32* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_KILL_CUT_TARGET_GET_H */
