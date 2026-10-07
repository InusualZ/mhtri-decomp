/* enemy/em029_prog.h - declarations `enemy/em029_prog.cpp` owns: enemy 029's action and sub-state handlers and its
 * program-table stubs (GUESS names, see the unit header).  C linkage: the map rows are plain names. */
#ifndef MHTRI_ENEMY_EM029_PROG_H
#define MHTRI_ENEMY_EM029_PROG_H

#include "types.h"
#include "enemy/ENEMY_WORK.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `em_action_dispatch` runs the handler for `_ENEMY_WORK::action`, each `em_actionN_dispatch` the one for
 * `state_sub`. */
void em_action_dispatch(struct _ENEMY_WORK* self);
void em_action0_dispatch(struct _ENEMY_WORK* self);
void em_action1_dispatch(struct _ENEMY_WORK* self);
void em_action2_dispatch(struct _ENEMY_WORK* self);
void em_action3_dispatch(struct _ENEMY_WORK* self);
void em_action4_dispatch(struct _ENEMY_WORK* self);
void em_action5_dispatch(struct _ENEMY_WORK* self);
void em_action6_dispatch(struct _ENEMY_WORK* self);
void em_action7_dispatch(struct _ENEMY_WORK* self);

/* The handlers the dispatchers above tail-call. */
void em_action0_step(struct _ENEMY_WORK* self);
void em_action1_sub0(struct _ENEMY_WORK* self);
void em_action1_sub1(struct _ENEMY_WORK* self);
void em_action1_sub2(struct _ENEMY_WORK* self, u8 mode);
void em_action1_sub3(struct _ENEMY_WORK* self);
void em_action1_sub4(struct _ENEMY_WORK* self);
void em_action1_sub5(struct _ENEMY_WORK* self);
void em_action1_sub6(struct _ENEMY_WORK* self);
void em_action1_sub7(struct _ENEMY_WORK* self);
void em_action1_sub8(struct _ENEMY_WORK* self);
void em_action2_sub0(struct _ENEMY_WORK* self, f32 value);
void em_action2_sub1(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub2(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub3(struct _ENEMY_WORK* self);
void em_action2_sub4(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub5(struct _ENEMY_WORK* self, f32 value);
void em_action3_sub0(struct _ENEMY_WORK* self, u8 mode);
void em_action3_sub1(struct _ENEMY_WORK* self);
void em_action3_sub2(struct _ENEMY_WORK* self);
void em_action6_sub0(struct _ENEMY_WORK* self);
void em_action6_sub1(struct _ENEMY_WORK* self);
void em_action7_step(struct _ENEMY_WORK* self);

/* Whether the area already holds an active team-19 enemy, the action band's hand-over test. */
u32 em_area_team_ck(u8 area);

/* Two stub rows of the program table: an empty one and a `return 0` one.  The runtime dump's names for them
 * (`DBClose`, `gdev_cc_shutdown`) are its junk mappings - both spellings appear at 295 and 133 addresses
 * respectively, i.e. the dump's import duplicated them - so they are named for their bodies and the band they sit
 * in instead (rule 7, evidence class 4). */
void em_action_nop(void);
s32 em_action_ret0(void);

/* The program table's damage slot: 1 when part 0 is undamaged. */
u32 em_parts_damage0_ck(struct _ENEMY_WORK* self, u8 part);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM029_PROG_H */
