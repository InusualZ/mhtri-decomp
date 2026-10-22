/* enemy/em_act_request_now.h - the declaration of `em_act_request_now`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ACT_REQUEST_NOW_H
#define MHTRI_ENEMY_EM_ACT_REQUEST_NOW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_act_request_now(struct _ENEMY_WORK* self, u32 a, u32 b);
#ifdef __cplusplus
}
#endif

#endif
