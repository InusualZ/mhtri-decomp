/* The enemy unit `enemy/fn_801DB8E0.cpp` (0x801DB8E0..0x801E0ADC): the enemy action band's
 * per-part damage/effect helpers.
 *
 * Declarations for the symbols of that range OTHER units call (docs/plan.md 6.5 rule 2: an extern
 * lives with the TU that owns the symbol).  `enemy/fn_801B0010.cpp` (the em030 program) carried
 * `fn_801E01BC` as a local declaration, written when its range was still an unregistered gap;
 * the unit's registration made it owned, so the declaration moved here.
 */
#ifndef MHTRI_ENEMY_FN_801DB8E0_H
#define MHTRI_ENEMY_FN_801DB8E0_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E01BC - "action 0xD with sub-state <= 5": r3 the work record and the answer in r3 (the em030
 * program's team-7 checker compares it against 1). */
u32 fn_801E01BC(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801DB8E0_H */
