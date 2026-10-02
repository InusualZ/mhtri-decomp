/* enemy/em_state_refresh.h - the declaration of `em_state_refresh`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_STATE_REFRESH_H
#define MHTRI_ENEMY_EM_STATE_REFRESH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_state_refresh(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
