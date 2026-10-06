/* enemy/em007_act.h - the declarations `enemy/em007_act.cpp` owns that other units call: the per-part
 * damage/effect helpers and its shared pool entries. */
#ifndef MHTRI_ENEMY_FN_801D80EC_H
#define MHTRI_ENEMY_FN_801D80EC_H

#include "types.h"

struct _ENEMY_WORK;

/* The unit's `.sdata2` pool entries `enemy/em005_act.cpp` also reads; declared, not defined. */
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

/* 0x801E01BC - "action 0xD with sub-state <= 5" (`enemy/em030_prog.cpp`'s team-7 checker compares the
 * answer against 1). */
u32 fn_801E01BC(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801D80EC_H */
