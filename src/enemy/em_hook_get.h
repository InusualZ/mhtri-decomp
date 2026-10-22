/* enemy/em_hook_get.h - the declaration of `em_hook_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HOOK_GET_H
#define MHTRI_ENEMY_EM_HOOK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void (*em_hook_get(struct _ENEMY_WORK* work, s32 index))(struct _ENEMY_WORK*);
#ifdef __cplusplus
}
#endif

#endif
