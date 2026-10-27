/* enemy/em_damage_record_clear.h - the declaration of `em_damage_record_clear`, which `enemy/fn_8011D448.cpp` owns (docs/plan.md 6.5
 * rule 2). */
#ifndef MHTRI_ENEMY_EM_DAMAGE_RECORD_CLEAR_H
#define MHTRI_ENEMY_EM_DAMAGE_RECORD_CLEAR_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8011EE84: clears the frame's damage record and latches the facing the hit directions are measured from. */
void em_damage_record_clear(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_DAMAGE_RECORD_CLEAR_H */
