/* The declarations `enemy/fn_8011D448.cpp` owns that other units call; `em_parts_damage_level_get` is declared at
 * C++ scope for its runtime-dump mangling.
 */
#ifndef MHTRI_ENEMY_FN_8011D448_H
#define MHTRI_ENEMY_FN_8011D448_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8011E7D8 - adds `amount` to the damage level of part `part`. */
void em_parts_damage_add(struct _ENEMY_WORK* self, u8 part, s32 amount);
/* 0x8011E9F0 - the state byte of part `part`. */
s8 em_parts_state_get(struct _ENEMY_WORK* self, u8 part);
/* 0x8011EA04 - breaks part `part`. */
void em_parts_break(struct _ENEMY_WORK* self, u8 part);
/* 0x8011EA90 - refreshes part `part` after its level changed; `flag` is the caller's 0. */
void em_parts_refresh(struct _ENEMY_WORK* self, u8 part, s32 flag);
/* 0x801206C8 and the three sibling applies below it - settle the enemy after an event; `flag` is the caller's 0/1. */
void em_event_settle(struct _ENEMY_WORK* self, s32 flag);
void em_event_settle_kind17(struct _ENEMY_WORK* self);
void em_event_settle_kind20(struct _ENEMY_WORK* self);
void em_event_settle_kind21(struct _ENEMY_WORK* self);
/* 0x8011E620 - raises status bits `mask` in the record's +0x824 word (GUESS name). */
void em_status_bits_set(struct _ENEMY_WORK* self, u32 mask);

#ifdef __cplusplus
}

/* r3 the work record, r4 the part index; returns that part's damage level (a byte). */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
#endif

#endif /* MHTRI_ENEMY_FN_8011D448_H */
