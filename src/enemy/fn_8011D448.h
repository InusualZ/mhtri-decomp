/* The enemy program/parts band `enemy/fn_8011D448.cpp` (0x8011D448..0x801251D0): the declarations
 * its consumers need (docs/plan.md 6.5 rule 2 - an extern lives with the TU that owns the symbol).
 * `em_parts_damage_level_get` is that unit's one function the runtime dump already names
 * (`em_parts_damage_level_get__FP11_ENEMY_WORKUc`), so it is declared at C++ scope with its real
 * signature (rule 9); its consumers call it as `em_parts_damage_level_get(self, part)`.
 *
 * Added with `enemy/fn_801A9540.cpp`'s registration: that range's `fn_801A960C`/`fn_801A9670` read
 * the per-part damage level through this function, and `src/ef/eft009.cpp` carried a private copy of
 * the declaration.  `enemy/fn_80181C88.cpp`'s `fn_80182080`/`fn_801825A4`/`fn_80182918` are a
 * second consumer (they declared it locally too), so this header is the one home for it.
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

#ifdef __cplusplus
}

/* r3 the work record, r4 the part index; returns that part's damage level (a byte). */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
#endif

#endif /* MHTRI_ENEMY_FN_8011D448_H */
