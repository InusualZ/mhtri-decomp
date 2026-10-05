/* enemy/em_hit_window_clear.h - the declaration of `em_hit_window_clear`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HIT_WINDOW_CLEAR_H
#define MHTRI_ENEMY_EM_HIT_WINDOW_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_hit_window_clear(struct _ENEMY_WORK *self, u32 a);
#ifdef __cplusplus
}
#endif

#endif
