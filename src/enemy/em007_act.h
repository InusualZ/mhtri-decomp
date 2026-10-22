/* enemy/em007_act.h - the declarations `enemy/em007_act.cpp` owns that other units call: the per-part
 * damage/effect helpers and its shared pool entries. */
#ifndef MHTRI_ENEMY_FN_801D80EC_H
#define MHTRI_ENEMY_FN_801D80EC_H

#include "types.h"

struct _ENEMY_WORK;

/* The unit's `.sdata2` pool entries `enemy/em005_act.cpp` also reads; declared, not defined. */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E01BC - "action 0xD with sub-state <= 5" (`enemy/em030_prog.cpp`'s team-7 checker compares the
 * answer against 1). */
u32 em007_act13_early_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801D80EC_H */
