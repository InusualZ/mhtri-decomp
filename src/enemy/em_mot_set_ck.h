/* enemy/em_mot_set_ck.h - the declaration of `em_mot_set_ck`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOT_SET_CK_H
#define MHTRI_ENEMY_EM_MOT_SET_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
#ifdef __cplusplus
}
#endif

#endif
