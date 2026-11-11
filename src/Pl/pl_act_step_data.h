/* The player act-step data unit `Pl/pl_act_step_data.cpp`: the `.data` tables the act state machine
 * (`Pl/pl_act.cpp`) indexes - step tables (`pl_steps_*`), the per-weapon-kind step-table pickers, the gun and shell
 * tables, the sharpness rows, the skill tables and the motion-number rows (`pl_motion_*`), and the `.bss` work rows
 * those pointers lead to.  `pl_act_kind4_item_ids` and `gun_level_bonus_tbl` have their own leaf headers.
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

/* The motion-number rows: `pl_motion_rows[id / 100][id % 100]` for ids below 1000, and
 * `pl_motion_kind_rows[kind][(id - 1000) / 100][(id - 1000) % 100]` above it (`kind` is `_PLW`+0x002; kinds 4-6 share a
 * row set).  The `_row2` rows are `.bss`, filled at run time. */
extern s16 pl_motion_kind0_row2[];
extern s16 pl_motion_kind1_row1[];
extern s16 pl_motion_kind1_row2[];
extern s16 pl_motion_kind2_row2[];
extern s16 pl_motion_kind3_row2[];
extern s16 pl_motion_kind456_row1[];
extern s16 pl_motion_kind456_row2[];
extern s16 pl_motion_kind7_row1[];
extern s16 pl_motion_kind7_row2[];
extern s16 pl_motion_kind8_row2[];
extern u32 pl_steps_idle[];
extern u32 pl_steps_turn[];
extern u32 pl_steps_hit_light[];
extern u32 pl_steps_hit_light_drawn[];
extern u32 pl_steps_hit_light_bow[];
extern u32 pl_steps_knockdown[];
extern u32 pl_steps_knockdown_drawn[];
extern u32 pl_steps_knockdown_bow[];
extern u32 pl_steps_hit_heavy[];
extern u32 pl_steps_hit_heavy_drawn[];
extern u32 pl_steps_hit_heavy_bow[];
extern u32 pl_steps_knockdown_heavy[];
extern u32 pl_steps_knockdown_heavy_drawn[];
extern u32 pl_steps_knockdown_heavy_bow[];
extern u32 pl_steps_stop[];
extern u32 pl_steps_drop_in[];
extern u32 pl_steps_run_turn[];
extern u32 pl_steps_dash[];
extern u32 pl_act_step_table_247[];
extern u32 pl_act_step_table_247_tail[];
extern u32 pl_steps_walk[];
extern u32 pl_steps_walk_end[];
extern u32 pl_steps_crouch[];
extern u32 pl_steps_stumble[];
extern u32 pl_steps_by_kind[];
extern u32 pl_steps_slot3_by_kind[];
extern u16 pl_item_icon_kind_tbl[];
extern u8 pl_shell_info_tbl[];
extern u8 pl_gun_ammo_entry_tbl[];
extern u8 pl_gun_clip_table[][0x2A];
extern u8 pl_shell_bure_tbl[];
extern u16 pl_sharpness_row_0[];
extern u16 pl_sharpness_row_1[];
extern u16 pl_sharpness_row_2[];
extern u16 pl_sharpness_row_3[];
extern u16 pl_sharpness_row_4[];
extern u8* pl_sharpness_tbl[];
extern u8 pl_shell_id_tbl[];
extern u16 pl_skill_valid_ids_a[];
extern u16 pl_skill_valid_ids_b[];
extern u8 pl_skill_rec_tbl[];
extern s16 pl_motion_row_0[];
extern s16 pl_motion_row_1[];
extern s16 pl_motion_row_2[];
extern s16 pl_motion_row_3[];
extern s16 pl_motion_row_4[];
extern s16 pl_motion_row_5[];
extern s16 pl_motion_row_6[];
extern s16 pl_motion_row_7[];
extern s16* pl_motion_rows[];
extern s16 pl_motion_kind0_row0[];
extern s16 pl_motion_kind0_row1[];
extern s16* pl_motion_kind0_rows[];
extern s16 pl_motion_kind1_row0[];
extern s16* pl_motion_kind1_rows[];
extern s16 pl_motion_kind2_row0[];
extern s16 pl_motion_kind2_row1[];
extern s16* pl_motion_kind2_rows[];
extern s16 pl_motion_kind3_row0[];
extern s16 pl_motion_kind3_row1[];
extern s16* pl_motion_kind3_rows[];
extern s16 pl_motion_kind456_row0[];
extern s16* pl_motion_kind456_rows[];
extern s16 pl_motion_kind7_row0[];
extern s16* pl_motion_kind7_rows[];
extern s16 pl_motion_kind8_row0[];
extern s16 pl_motion_kind8_row1[];
extern s16* pl_motion_kind8_rows[];
extern s16** pl_motion_kind_rows[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STEP_DATA_H */
