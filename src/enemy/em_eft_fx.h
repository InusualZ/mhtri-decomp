/* The declarations `enemy/em_eft_fx.cpp` owns that other units call. */
#ifndef MHTRI_ENEMY_EM_EFT_FX_H
#define MHTRI_ENEMY_EM_EFT_FX_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8011D448 */
void em_eft_fx_spawn(struct _ENEMY_WORK* self, u32 type, u32 id, u32 timer, f32 scale);
/* 0x8011D4FC */
void em_eft_fx_spawn_pair(struct _ENEMY_WORK* self, u32 type, u32 id0, u32 id1, u32 timer, f32 scale);
/* 0x8011D690 */
void em_eft_fx_spawn_anchor(struct _ENEMY_WORK* self, u32 type, u32 id, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_EFT_FX_H */
