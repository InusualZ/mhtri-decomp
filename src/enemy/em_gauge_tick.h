/* enemy/em_gauge_tick.h - the declaration of `em_gauge_tick`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_GAUGE_TICK_H
#define MHTRI_ENEMY_EM_GAUGE_TICK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_gauge_tick(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
#ifdef __cplusplus
}
#endif

#endif
