/* Declarations owned by `Pl/pl_act_class3.cpp` (rule 2): the act step tables of its `.data` run that
 * `Pl/pl_act_step.cpp`'s weapon acts hand `Pl_act_set_step_table`.  Declared, never defined: the bytes are the owner's.
 */
#ifndef MHTRI_PL_PL_ACT_CLASS3_H
#define MHTRI_PL_PL_ACT_CLASS3_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805EA804 - the knockback act's alternate step table for weapon kind 3 (registry name, GUESS). */
extern u32 pl_knockback_steps_bow_alt[];
/* 0x805EB6E0 - the evade act's alternate step table for weapon kind 3 (registry name, GUESS). */
extern u32 pl_evade_steps_bow_alt[];

/* 0x805EA0D8: the guard act's step table for weapon kind 3 */
extern u32 pl_steps_guard_kind3[];
/* 0x805EAF50: the guard-walk act's step table for weapon kind 3 */
extern u32 pl_steps_guard_walk_kind3[];
/* 0x805EA188: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_aim_kind3[];
/* 0x805EB000: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_aim_walk_kind3[];
/* 0x8034EE18: the class-3 weapon act dispatcher */
void pl_weapon3_act_dispatch(struct _PLW* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_CLASS3_H */
