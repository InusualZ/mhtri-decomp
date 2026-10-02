/* enemy/em_move_mode_set.h - the declaration of `em_move_mode_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOVE_MODE_SET_H
#define MHTRI_ENEMY_EM_MOVE_MODE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
#ifdef __cplusplus
}
#endif

#endif
