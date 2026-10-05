/* enemy/em_turn_in_window.h - the declaration of `em_turn_in_window`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_TURN_IN_WINDOW_H
#define MHTRI_ENEMY_EM_TURN_IN_WINDOW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);
#ifdef __cplusplus
}
#endif

#endif
