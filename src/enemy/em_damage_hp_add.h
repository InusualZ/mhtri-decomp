/* enemy/em_damage_hp_add.h - leaf header: the enemy damage-record writers `enemy/fn_8011D448.cpp` owns (C linkage):
 * `em_damage_hp_add` (0x8011EF24), `em_damage_attr_add` (0x8011EFBC), `em_damage_part_hit_set` (0x8011F14C),
 * `em_damage_stun_add` (0x8011F16C), `em_damage_flags_set` (0x8011F230) and `em_damage_hate_add` (0x8011F240). */
#ifndef MHTRI_ENEMY_EM_DAMAGE_HP_ADD_H
#define MHTRI_ENEMY_EM_DAMAGE_HP_ADD_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
void em_damage_hp_add(struct _ENEMY_WORK* self, u8 part, s16 damage, u8 heavy);
void em_damage_attr_add(struct _ENEMY_WORK* self, u8 part, u8 kind, s16 base, s16 value, u8 heavy);
void em_damage_part_hit_set(struct _ENEMY_WORK* self, u8 part);
void em_damage_stun_add(struct _ENEMY_WORK* self, u8 unused, u8 attacker, u8 value);
void em_damage_flags_set(struct _ENEMY_WORK* self, u16 mask);
void em_damage_hate_add(struct _ENEMY_WORK* self, u8 side, u8 index, s16 damage, u8 heavy);
#ifdef __cplusplus
}
#endif

#endif
