/* The enemy program/parts band `enemy/fn_8011D448.cpp` (0x8011D448..0x801251D0): the declarations
 * its consumers need (docs/plan.md 6.5 rule 2 - an extern lives with the TU that owns the symbol).
 * `em_parts_damage_level_get` is that unit's one function the runtime dump already names
 * (`em_parts_damage_level_get__FP11_ENEMY_WORKUc`), so it is declared at C++ scope with its real
 * signature (rule 9); its consumers call it as `em_parts_damage_level_get(self, part)`.
 *
 * Added with `enemy/fn_801A9540.cpp`'s registration: that range's `fn_801A960C`/`fn_801A9670` read
 * the per-part damage level through this function, and `src/ef/eft009.cpp` carried a private copy of
 * the declaration.
 */
#ifndef MHTRI_ENEMY_FN_8011D448_H
#define MHTRI_ENEMY_FN_8011D448_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
/* r3 the work record, r4 the part index; returns that part's damage level (a byte). */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
#endif

#endif /* MHTRI_ENEMY_FN_8011D448_H */
