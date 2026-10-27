/* enemy/em_sight2_get.h - leaf header: `em_sight2_get` (0x80126454), owned by `enemy/em_common.cpp`. */
#ifndef MHTRI_ENEMY_EM_SIGHT2_GET_H
#define MHTRI_ENEMY_EM_SIGHT2_GET_H
#include "types.h"
struct _ENEMY_WORK;
#ifdef __cplusplus
extern "C" {
#endif
/* Returns the enemy's second sight-range table (f32 entries). */
f32* em_sight2_get(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif
#endif /* MHTRI_ENEMY_EM_SIGHT2_GET_H */
