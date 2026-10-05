/* The enemy unit `enemy/em007_act.cpp` (the em007 TU, 0x801D71C4..0x801E0ADC; the header was
 * `enemy/fn_801DB8E0.h` until that unit folded in): the enemy action band's
 * per-part damage/effect helpers.
 *
 * Declarations for the symbols of that range OTHER units call (docs/plan.md 6.5 rule 2: an extern
 * lives with the TU that owns the symbol).  `enemy/fn_801B0010.cpp` (the em030 program) carried
 * `fn_801E01BC` as a local declaration, written when its range was still an unregistered gap;
 * the unit's registration made it owned, so the declaration moved here.
 */
#ifndef MHTRI_ENEMY_FN_801D80EC_H
#define MHTRI_ENEMY_FN_801D80EC_H

#include "types.h"

struct _ENEMY_WORK;

/* The `.sdata2` float pool entries this unit owns that `enemy/em005_act.cpp` reads as well (one TU's code split
 * across the units).  Declared, never defined: the pool belongs to the data pass. */
extern f32 lbl_807994F8;
extern f32 lbl_807994FC;
extern f32 lbl_80799500;
extern f32 lbl_80799504;
extern f32 lbl_80799508;
extern f32 lbl_8079950C;
extern f32 lbl_80799510;
extern f32 lbl_80799514;
extern f32 lbl_80799518;
extern f32 lbl_8079951C;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E01BC - "action 0xD with sub-state <= 5": r3 the work record and the answer in r3 (the em030
 * program's team-7 checker compares it against 1). */
u32 fn_801E01BC(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801D80EC_H */
