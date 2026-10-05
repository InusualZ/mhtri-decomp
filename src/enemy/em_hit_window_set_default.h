/* enemy/em_hit_window_set_default.h - the declaration of `em_hit_window_set_default`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HIT_WINDOW_SET_DEFAULT_H
#define MHTRI_ENEMY_EM_HIT_WINDOW_SET_DEFAULT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
#ifdef __cplusplus
}
#endif

#endif
