/* enemy/em_char_pose_sync.h - the declaration of `em_char_pose_sync`, which `enemy/fn_80138074.cpp` owns (docs/plan.md 6.5
 * rule 2).  GUESS name (the owner's body). */
#ifndef MHTRI_ENEMY_EM_CHAR_POSE_SYNC_H
#define MHTRI_ENEMY_EM_CHAR_POSE_SYNC_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80138E64: copies the work's scale, rotation and position into its embedded `MHchar`. */
void em_char_pose_sync(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_CHAR_POSE_SYNC_H */
