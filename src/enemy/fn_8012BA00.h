/* The declaration of `enemy/em_common.cpp`'s motion frame cost `fn_8012BA00`, in the signature its consumers use. */
#ifndef MHTRI_ENEMY_FN_8012BA00_H
#define MHTRI_ENEMY_FN_8012BA00_H

#include "types.h"
#include "nw4r/math.h"

struct EnemyActionTable;
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8012BA00(struct _ENEMY_WORK* enemy, struct EnemyActionTable* table, void* work, s32 kind, u16 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8012BA00_H */
