/* enemy/em_se_tbl_play.h - the declaration of `em_se_tbl_play`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_SE_TBL_PLAY_H
#define MHTRI_ENEMY_EM_SE_TBL_PLAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
#ifdef __cplusplus
}
#endif

#endif
