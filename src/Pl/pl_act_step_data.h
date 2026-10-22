/* The player act-step data unit `Pl/pl_act_step_data.cpp`: the `.data` tables the act state machine
 * (`Pl/pl_act.cpp`) indexes by act number - skill-id and step tables, per-act timing words and effect-pair tables.
 * Declared, never defined: the bytes are the data unit's own.
 */
#ifndef MHTRI_PL_PL_ACT_STEP_DATA_H
#define MHTRI_PL_PL_ACT_STEP_DATA_H

#include "types.h"

/* One meal skill row of `meal_skill_tbl` (0x805BFFF8, 52 rows): two skills and their values, read by the lobby
 * kitchen (`lobby/lb_quest_ui.cpp`). size: 0x8 */
typedef struct MealSkill {
    /* +0x0 */ u16 kind_a;
    /* +0x2 */ s16 value_a;
    /* +0x4 */ u16 kind_b;
    /* +0x6 */ s16 value_b;
} MealSkill;

#ifdef __cplusplus
extern "C" {
#endif

extern MealSkill meal_skill_tbl[52];

extern u16 lbl_805C0198[];
extern u16 lbl_805C01B8[];
extern u8 lbl_805C01C8[];
extern u8* pl_sharpness_tbl[];
extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u8 lbl_805BF5E0[];
extern u8 lbl_805BF538[];
extern u8 lbl_805BFCD8[];
extern u8 lbl_805BFFCC[];

/* 0x805BEBEC (0x152) - the step table pl_act_step_arm_table hands Pl_act_set_step_table. */
extern u32 pl_act_step_table_247[];

/* 0x805BDC48: the step table the rest act hands Pl_act_set_step_table */
extern u32 pl_steps_idle[];
/* 0x805BEFE4: the step table the crouch act's hide variant hands Pl_act_set_step_table */
extern u32 pl_steps_crouch[];
/* 0x805BDF78: the step table the turn acts hand Pl_act_set_step_table */
extern u32 pl_steps_turn[];
/* 0x805BEE14: the step table the walk act hands Pl_act_set_step_table */
extern u32 pl_steps_walk[];
/* 0x805BF278: the step table the stumble stop hands Pl_act_set_step_table */
extern u32 pl_steps_stumble[];
/* 0x805BEA60: the step table the dash acts hand Pl_act_set_step_table */
extern u32 pl_steps_dash[];
/* 0x805BE238: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_light[];
/* 0x805BE3F8: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_heavy[];
/* 0x805BE2D4: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_light_bow[];
/* 0x805BE494: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_heavy_bow[];
/* 0x805BE290: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_light_drawn[];
/* 0x805BE450: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_hit_heavy_drawn[];
/* 0x805BE318: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown[];
/* 0x805BE4D8: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown_heavy[];
/* 0x805BE3B4: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown_bow[];
/* 0x805BE574: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown_heavy_bow[];
/* 0x805BE370: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown_drawn[];
/* 0x805BE530: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_knockdown_heavy_drawn[];
/* 0x805BE7E0: a step table `Pl/pl_act_step.cpp` hands `Pl_act_set_step_table` */
extern u32 pl_steps_drop_in[];
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STEP_DATA_H */
