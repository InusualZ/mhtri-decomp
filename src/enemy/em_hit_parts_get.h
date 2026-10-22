/* `em_hit_parts_get`, defined by `enemy/em_common.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_EM_HIT_PARTS_GET_H
#define MHTRI_ENEMY_EM_HIT_PARTS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80126104 (0x24): the enemy's hit-part list (its data record's +0x24 word). */
struct _ENEMY_WORK;
s32 em_hit_parts_get(struct _ENEMY_WORK* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_HIT_PARTS_GET_H */
