/* `em_tall_ck`, defined by `enemy/em_common.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_EM_TALL_CK_H
#define MHTRI_ENEMY_EM_TALL_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8012F39C (0x58): whether a kind-4 enemy stands above 0.9 of its model height. */
struct _ENEMY_WORK;
s32 em_tall_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_TALL_CK_H */
