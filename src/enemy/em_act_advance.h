/* enemy/em_act_advance.h - the declaration of `em_act_advance`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ACT_ADVANCE_H
#define MHTRI_ENEMY_EM_ACT_ADVANCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_act_advance(struct _ENEMY_WORK* self, u32 kind);
#ifdef __cplusplus
}
#endif

#endif
