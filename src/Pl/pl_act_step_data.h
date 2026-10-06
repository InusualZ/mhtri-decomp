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
extern u8* lbl_805BFFA8[];
extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u8 lbl_805BF5E0[];
extern u8 lbl_805BF538[];
extern u8 lbl_805BFCD8[];
extern u8 lbl_805BFFCC[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STEP_DATA_H */
