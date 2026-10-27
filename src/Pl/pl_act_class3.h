/* Declarations owned by `Pl/pl_act_class3.cpp` (rule 2): its act handlers, the row type its row acts walk, and the
 * tables of its `.data`/`.sdata` run.  The `.data` tables are declared, not yet emitted (the unit header's residual).
 */
#ifndef MHTRI_PL_PL_ACT_CLASS3_H
#define MHTRI_PL_PL_ACT_CLASS3_H

#include "types.h"

struct _PLW;

/* One half of a weapon-3 act row: the motion `Pl_chr_set_attr_default` sets and its two arguments.
 * size: 0xC (`mulli r0,<half>,12` in `pl_weapon3_row_apply`) */
typedef struct PlWeapon3RowHalf {
    /* +0x0 */ u16 motion;     /* 0xFFFF in the first half: the row starts at its second half */
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ s32 arg_a;
    /* +0x8 */ s32 arg_b;
} PlWeapon3RowHalf;

/* One weapon-3 act row: a lead-in half and a main half, the step table and the attack value, each tagged with
 * the half that applies it.  size: 0x20 (the row acts index a table with `slwi <row>,5`) */
typedef struct PlWeapon3Row {
    /* +0x00 */ PlWeapon3RowHalf half[2];
    /* +0x18 */ u32* step_table;  /* handed to `Pl_act_set_step_table` */
    /* +0x1C */ u8 attack_half;   /* the half whose application arms `attack` */
    /* +0x1D */ u8 step_half;     /* the half whose application sets `step_table` */
    /* +0x1E */ s16 attack;       /* the signed attack value `pl_weapon3_attack_set` takes */
} PlWeapon3Row;

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

/* The weapon-3 attack entries' hit table and the two hit-parameter records its `.sdata` id list points at, then the
 * act step tables (`Pl_act_set_step_table` input; the row tables and `Pl/pl_act_step.cpp`'s per-weapon pointer
 * tables `pl_knockback_bow_steps`/`pl_evade_bow_steps` and `Pl/pl_act_step_data.cpp` point at them), as words. */
extern u32 pl_weapon3_hit_params_a[];
extern u32 pl_weapon3_hit_params_b[];
extern u32 pl_weapon3_hit_table[];
extern u32 pl_weapon3_steps_combo_5[];
extern u32 pl_weapon3_steps_combo_0[];
extern u32 pl_weapon3_steps_combo_2[];
extern u32 pl_weapon3_steps_combo_3[];
extern u32 pl_weapon3_steps_charge[];
extern u32 pl_weapon3_steps_guard_swing[];
extern u32 pl_weapon3_steps_step_back[];
extern u32 pl_weapon3_steps_hold_release[];
extern u32 pl_weapon3_steps_dash[];
extern u32 pl_weapon3_steps_shell[];
extern u32 pl_weapon3_steps_finish_0[];
extern u32 pl_weapon3_steps_sweep_0[];
extern u32 pl_weapon3_steps_sweep_2[];
extern u32 pl_weapon3_steps_sweep_3[];
extern u32 pl_weapon3_steps_quake[];
extern u32 pl_weapon3_knockback_steps_2[];
extern u32 pl_weapon3_knockback_steps_3[];
extern u32 pl_weapon3_knockback_steps_0[];
extern u32 pl_weapon3_knockback_steps_1[];
extern u32 pl_weapon3_knockback_steps_4[];
extern u32 pl_weapon3_knockback_steps_5[];
extern u32 pl_weapon3_alt_steps_combo_5[];
extern u32 pl_weapon3_alt_steps_combo_0[];
extern u32 pl_weapon3_alt_steps_combo_2[];
extern u32 pl_weapon3_alt_steps_combo_3[];
extern u32 pl_weapon3_alt_steps_charge[];
extern u32 pl_weapon3_alt_steps_guard_swing[];
extern u32 pl_weapon3_alt_steps_step_back[];
extern u32 pl_weapon3_alt_steps_hold_release[];
extern u32 pl_weapon3_alt_steps_dash[];
extern u32 pl_weapon3_alt_steps_shell[];
extern u32 pl_weapon3_alt_steps_finish_0[];
extern u32 pl_weapon3_alt_steps_sweep_0[];
extern u32 pl_weapon3_alt_steps_sweep_2[];
extern u32 pl_weapon3_alt_steps_sweep_3[];
extern u32 pl_weapon3_alt_steps_quake[];
extern u32 pl_weapon3_evade_steps_2[];
extern u32 pl_weapon3_evade_steps_3[];
extern u32 pl_weapon3_evade_steps_0[];
extern u32 pl_weapon3_evade_steps_1[];
extern u32 pl_weapon3_evade_steps_4[];
extern u32 pl_weapon3_evade_steps_5[];
extern u32 pl_weapon3_steps_shared_a[];
extern u32 pl_weapon3_steps_shared_b[];
/* The row tables of the four row acts, one per weapon form (0 normal, 1 alternate). */
extern PlWeapon3Row pl_weapon3_rows_combo[10];
extern PlWeapon3Row pl_weapon3_alt_rows_combo[10];
extern PlWeapon3Row pl_weapon3_rows_thrust[7];
extern PlWeapon3Row pl_weapon3_alt_rows_thrust[7];
extern PlWeapon3Row pl_weapon3_rows_finish[1];
extern PlWeapon3Row pl_weapon3_alt_rows_finish[1];
extern PlWeapon3Row pl_weapon3_rows_sweep[4];
extern PlWeapon3Row pl_weapon3_alt_rows_sweep[4];

/* 0x8034EE18: the class-3 weapon act dispatcher */
void pl_weapon3_act_dispatch(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_CLASS3_H */
