/* enemy/em_status_ck.h - the declaration of `em_status_ck`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_STATUS_CK_H
#define MHTRI_ENEMY_EM_STATUS_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_status_ck(struct _ENEMY_WORK* enemy, s32 value);
#ifdef __cplusplus
}
#endif

#endif
