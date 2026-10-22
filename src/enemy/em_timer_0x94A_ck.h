/* enemy/em_timer_0x94A_ck.h - the declaration of `em_timer_0x94A_ck`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5
 * rule 2); the signature is the one its callers use (`enemy/em003_prog.cpp` tests the result against 0). */
#ifndef MHTRI_ENEMY_EM_TIMER_0X94A_CK_H
#define MHTRI_ENEMY_EM_TIMER_0X94A_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_timer_0x94A_ck(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
