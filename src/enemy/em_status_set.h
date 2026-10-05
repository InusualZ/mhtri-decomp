/* enemy/em_status_set.h - the declaration of `em_status_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_STATUS_SET_H
#define MHTRI_ENEMY_EM_STATUS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_status_set(struct _ENEMY_WORK* self, s32 kind);
#ifdef __cplusplus
}
#endif

#endif
