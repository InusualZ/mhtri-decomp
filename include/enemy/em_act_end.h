/* enemy/em_act_end.h - the declaration of `em_act_end`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ACT_END_H
#define MHTRI_ENEMY_EM_ACT_END_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_act_end(struct _ENEMY_WORK* self, u32 mode);
#ifdef __cplusplus
}
#endif

#endif
