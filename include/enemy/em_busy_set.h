/* enemy/em_busy_set.h - the declaration of `em_busy_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_BUSY_SET_H
#define MHTRI_ENEMY_EM_BUSY_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_busy_set(struct _ENEMY_WORK* enemy);
#ifdef __cplusplus
}
#endif

#endif
