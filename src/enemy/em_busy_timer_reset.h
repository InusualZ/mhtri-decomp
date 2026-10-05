/* enemy/em_busy_timer_reset.h - the declaration of `em_busy_timer_reset`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_BUSY_TIMER_RESET_H
#define MHTRI_ENEMY_EM_BUSY_TIMER_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_busy_timer_reset(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
