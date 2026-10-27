/*
 * Pl/pl_act_class3.cpp - the player act-state handlers of `_PLW::field_0x002` class 3 and their dispatcher
 *   `pl_weapon3_act_dispatch`.
 * RANGE. .text 0x8034D2B0-0x8034F138 (24 functions); .data 0x805E9248-0x805EBBE0 (the act tables), .sdata
 *   0x80793308-0x80793330 (five 8-byte table-pair words), .sdata2 0x8079B3A8-0x8079B3C8, extab, extabindex.
 *   `pl_weapon3_act_dispatch` (0x320 B) switches on `_PLW::act_no` (`act_no <= 0x43`, `jumptable_805E9AD8`) and tail-calls the
 *   handlers with a (row, form) pair; its only caller is `Pl/pl_act_step.cpp`'s `fn_8025DE38` (`b pl_weapon3_act_dispatch` at
 *   0x8025DEB0), whose `jumptable_805C5648` picks one dispatcher per `field_0x002`: 0 `pl_weapon0_act_dispatch`, 1 `pl_weapon1_act_dispatch`,
 *   2 `pl_weapon2_act_dispatch`, 3 `pl_weapon3_act_dispatch`, 4-6 `pl_weapon456_act_dispatch`, 7 `pl_act_class7_dispatch`, 8 `Pl_act_frame_dispatch`.
 *   Left seam 0x8034D2B0: the `extabindex` run (`place_list_highlight_draw` is the menu unit's last record), the `.sdata2` run
 *   (0x8079B3A0 is the menu unit's last word) and the callee set all break there.  Right seam 0x8034F138:
 *   `em024_action11_state5_ck` reads `_ENEMY_WORK` and its only caller (`camera/camera_main.cpp`'s `camera_kill_cut_start`)
 *   gates it on enemy id 0x18, so it opens `enemy/em024_ai.cpp`.
 *   The `.data` run is one object: emitted in source order, the dispatcher's jump table lands at +0x890 exactly where
 *   the target has it, between the row tables (defined before the functions) and the step tables (defined after).
 *   The step tables `Pl/pl_act_step.cpp`'s `pl_knockback_bow_steps`/`pl_evade_bow_steps`, its guard/aim acts and
 *   `Pl/pl_act_step_data.cpp` point at are byte-coded records; `config.yml` blocks the relocations dtk read into them
 *   (0x805E9BE8-0x805EBBE0: 38 words such as 0x80160004 relocated against `em008_act7_0`/`em006_motion_events`/
 *   `em011_motion_events`).
 * FLAGS. The unit's `cflags` (`-O3 -inline noauto -opt nopeephole`, C++ exceptions on).
 * NAMES. The file name is a GUESS: the selector's meaning is unproven (the weapon type is the likely one - values 4-6
 *   share one dispatcher, the shape of three ranged weapons), so the name says `class3`.  GUESS (from each body, its
 *   dispatcher cases and its callees): every `pl_weapon3_*` function, `PlWeapon3Row`/`PlWeapon3RowHalf`,
 *   the data rows `pl_weapon3_*` and `_PLW::rehit_timer_0x2C`; registry names (GUESS) for the foreign callees
 *   `pl_sub_gauge_skill_add` (0x80276B58) and `pl_sub_gauge_level_cost` (0x80264A28).
 * RESIDUALS. Every row is written.
 *  - Data: `.data` (0x2998) and `.sdata` (0x28) are emitted byte-equal; `.sdata2` is emitted as literals (0x1C of the
 *    claimed 0x20: the last 4 bytes are the section's alignment tail).  The hit records and step tables are written
 *    as words (their record layouts are not decoded).
 *  - flipcheck: the two registry callees have no map row until the registry lands.
 *  - The `.sdata2` constants pair as `@N` against the map's `lbl_8079B3A8..C0` (names only).
 * SHAPES. The dispatcher's case bodies follow the retail emission order (17, 19, 20, 18 and 66, 67, 64, 65).
 */

#include "types.h"
#include "pl.h"
#include "Pl/pl_act_class3.h"
#include "Pl/pl_act.h"
#include "Pl/pl_act_step.h"
#include "Pl/fn_802693C4.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act_gauge_gate.h"
#include "Pl/pl_hit_result_best_get.h"
#include "Pl/pl_now_area_ck.h"
#include "Pl/pl_sub_gauge_skill_add.h"
#include "Pl/pl_sub_gauge_level_cost.h"
#include "camera/camera_main.h"
#include "ef/eft029_fx.h"
#include "menu/hit_mask_ck.h"
#include "sound/se_req.h"

extern "C" {

/* The hit-parameter records, the hit table and the row tables; the jump table of `pl_weapon3_act_dispatch`
 * follows them in `.data`. */
u32 pl_weapon3_hit_params_a[20] = {
    0x00070000, 0x00030000, 0x00000000, 0x42200000,
    0xC0666666, 0x41F80000, 0x40ACCCCD, 0x42200000,
    0x00000000, 0x43700000, 0xFFFF0000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_hit_params_b[20] = {
    0x000B000B, 0x00010000, 0x00000000, 0x42540000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x42200000, 0xFFFF0000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_hit_table[156] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000014, 0x000A0017,
    0x01000032, 0x00020000, 0x00001C00, 0x00000402,
    0x00000000, 0x0024000A, 0x001C0100, 0x00320002,
    0x00000000, 0x1C00EC04, 0x06020000, 0x00000014,
    0x000A001E, 0x01000032, 0x00020000, 0x00001C00,
    0x00000402, 0x00000000, 0x00100008, 0x001C0100,
    0x00320002, 0x00000000, 0x1C000000, 0x04020000,
    0x00000010, 0x0008001E, 0x01000032, 0x00020000,
    0x00001C00, 0x00000402, 0x00000000, 0x000A0004,
    0x00140100, 0x00320002, 0x00000000, 0x1C000000,
    0x02020000, 0x00000000, 0x001A0010, 0x02000032,
    0x00020000, 0x00001D00, 0x00000002, 0x00000000,
    0x000C0006, 0x00280200, 0x00320002, 0x00000000,
    0x1C000000, 0x06020000, 0x0000003C, 0x001E0014,
    0x02000032, 0x00020000, 0x00001D00, 0x00000002,
    0x00000000, 0x0010000E, 0x00280200, 0x00320002,
    0x00000000, 0x1C000000, 0x06020000, 0x0000000A,
    0x00080014, 0x01000032, 0x00020000, 0x00001C00,
    0x00000202, 0x00000000, 0x0014000A, 0x001E0100,
    0x00320002, 0x00000000, 0x1C000000, 0x04020000,
    0x00000020, 0x00100014, 0x01000032, 0x00020000,
    0x00000100, 0x00020402, 0x00000000, 0x000A0008,
    0x000E0100, 0x001E0001, 0x00000001, 0x010A1B00,
    0x00020000, 0x00000014, 0x000A0017, 0x01000032,
    0x00020000, 0x00001C00, 0x00000402, 0x00000000,
    0x0024000A, 0x001C0100, 0x00320002, 0x00000000,
    0x1C00EC04, 0x06020000, 0x00000014, 0x000A001E,
    0x01000032, 0x00020000, 0x00001C00, 0x00000402,
    0x00000000, 0x00100008, 0x001C0100, 0x00320002,
    0x00000000, 0x1C000000, 0x04020000, 0x00000010,
    0x0008001E, 0x01000032, 0x00020000, 0x00001C00,
    0x00000402, 0x00000000, 0x0000001A, 0x00100200,
    0x00320002, 0x00000000, 0x1D000000, 0x00020000,
    0x00000014, 0x000A001E, 0x01000032, 0x00020000,
    0x00001C00, 0x00000402, 0x00000000, 0x00200010,
    0x00140100, 0x00320002, 0x00000000, 0x01000002,
    0x04020000, 0x0000000A, 0x0008000E, 0x0100001E,
    0x00010000, 0x0001010A, 0x1B000002, 0x00000000,
};
PlWeapon3Row pl_weapon3_rows_combo[10] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x451, { 0, 0 }, 4, 0 } }, pl_weapon3_steps_combo_0, 1, 1, 1 },
    { { { 0x46A, { 0, 0 }, 2, 0 }, { 0x451, { 0, 0 }, 2, 34 } }, pl_weapon3_steps_combo_0, 0, 1, -2 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x451, { 0, 0 }, 6, 10 } }, pl_weapon3_steps_combo_2, 1, 1, 1 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x451, { 0, 0 }, 6, 10 } }, pl_weapon3_steps_combo_3, 1, 1, 3 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x451, { 0, 0 }, 4, 10 } }, pl_weapon3_steps_combo_0, 1, 1, 1 },
    { { { 0x464, { 0, 0 }, 0, 0 }, { 0x451, { 0, 0 }, 0, 20 } }, pl_weapon3_steps_combo_5, 1, 1, 1 },
    { { { 0x45F, { 0, 0 }, 4, 0 }, { 0x451, { 0, 0 }, 0, 20 } }, pl_weapon3_steps_combo_5, 1, 1, 1 },
    { { { 0x454, { 0, 0 }, 4, 0 }, { 0x451, { 0, 0 }, 0, 16 } }, pl_weapon3_steps_combo_5, 1, 1, 1 },
    { { { 0x454, { 0, 0 }, 4, 0 }, { 0x451, { 0, 0 }, 0, 16 } }, pl_weapon3_steps_combo_2, 1, 1, 1 },
    { { { 0x454, { 0, 0 }, 4, 0 }, { 0x451, { 0, 0 }, 0, 16 } }, pl_weapon3_steps_combo_3, 1, 1, 3 },
};
PlWeapon3Row pl_weapon3_alt_rows_combo[10] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x483, { 0, 0 }, 4, 0 } }, pl_weapon3_alt_steps_combo_0, 1, 1, 15 },
    { { { 0x49C, { 0, 0 }, 2, 0 }, { 0x483, { 0, 0 }, 2, 34 } }, pl_weapon3_alt_steps_combo_0, 0, 1, -16 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x483, { 0, 0 }, 4, 10 } }, pl_weapon3_alt_steps_combo_2, 1, 1, 15 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x483, { 0, 0 }, 4, 10 } }, pl_weapon3_alt_steps_combo_3, 1, 1, 17 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x483, { 0, 0 }, 4, 10 } }, pl_weapon3_alt_steps_combo_0, 1, 1, 15 },
    { { { 0x496, { 0, 0 }, 0, 0 }, { 0x483, { 0, 0 }, 0, 20 } }, pl_weapon3_alt_steps_combo_5, 1, 1, 15 },
    { { { 0x491, { 0, 0 }, 4, 0 }, { 0x483, { 0, 0 }, 0, 20 } }, pl_weapon3_alt_steps_combo_5, 1, 1, 15 },
    { { { 0x494, { 0, 0 }, 8, 4 }, { 0x483, { 0, 0 }, 0, 20 } }, pl_weapon3_alt_steps_combo_5, 1, 1, 15 },
    { { { 0x486, { 0, 0 }, 4, 0 }, { 0x483, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_2, 1, 1, 15 },
    { { { 0x486, { 0, 0 }, 4, 0 }, { 0x483, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_3, 1, 1, 17 },
};
PlWeapon3Row pl_weapon3_rows_thrust[7] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x452, { 0, 0 }, 4, 0 } }, pl_weapon3_steps_combo_0, 1, 1, 4 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x452, { 0, 0 }, -6, 10 } }, pl_weapon3_steps_combo_2, 1, 1, 4 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x452, { 0, 0 }, -6, 10 } }, pl_weapon3_steps_combo_3, 1, 1, 5 },
    { { { 0x462, { 0, 0 }, 8, 4 }, { 0x452, { 0, 0 }, 0, 16 } }, pl_weapon3_steps_combo_5, 1, 1, 4 },
    { { { 0x463, { 0, 0 }, 0, 0 }, { 0x452, { 0, 0 }, 0, 16 } }, pl_weapon3_steps_combo_5, 1, 1, 4 },
    { { { 0x455, { 0, 0 }, 4, 0 }, { 0x452, { 0, 0 }, 0, 12 } }, pl_weapon3_steps_combo_2, 1, 1, 4 },
    { { { 0x455, { 0, 0 }, 4, 0 }, { 0x452, { 0, 0 }, 0, 12 } }, pl_weapon3_steps_combo_3, 1, 1, 5 },
};
PlWeapon3Row pl_weapon3_alt_rows_thrust[7] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x484, { 0, 0 }, 4, 0 } }, pl_weapon3_alt_steps_combo_0, 1, 1, 18 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x484, { 0, 0 }, -6, 10 } }, pl_weapon3_alt_steps_combo_2, 1, 1, 18 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x484, { 0, 0 }, -6, 10 } }, pl_weapon3_alt_steps_combo_3, 1, 1, 19 },
    { { { 0x494, { 0, 0 }, 8, 4 }, { 0x484, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_5, 1, 1, 18 },
    { { { 0x495, { 0, 0 }, 0, 0 }, { 0x484, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_5, 1, 1, 18 },
    { { { 0x486, { 0, 0 }, 4, 0 }, { 0x484, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_2, 1, 1, 18 },
    { { { 0x486, { 0, 0 }, 4, 0 }, { 0x484, { 0, 0 }, 0, 16 } }, pl_weapon3_alt_steps_combo_3, 1, 1, 19 },
};
PlWeapon3Row pl_weapon3_rows_finish[1] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x45C, { 0, 0 }, 2, 0 } }, pl_weapon3_steps_finish_0, 1, 1, 6 },
};
PlWeapon3Row pl_weapon3_alt_rows_finish[1] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x48E, { 0, 0 }, 2, 0 } }, pl_weapon3_alt_steps_finish_0, 1, 1, 11 },
};
PlWeapon3Row pl_weapon3_rows_sweep[4] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x453, { 0, 0 }, 4, 0 } }, pl_weapon3_steps_sweep_0, 1, 1, 13 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x453, { 0, 0 }, 4, 4 } }, pl_weapon3_steps_sweep_0, 1, 1, 13 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x453, { 0, 0 }, 4, 4 } }, pl_weapon3_steps_sweep_2, 1, 1, 13 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x453, { 0, 0 }, 4, 4 } }, pl_weapon3_steps_sweep_3, 1, 1, 13 },
};
PlWeapon3Row pl_weapon3_alt_rows_sweep[4] = {
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x485, { 0, 0 }, 4, 0 } }, pl_weapon3_alt_steps_sweep_0, 1, 1, 22 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x485, { 0, 0 }, 4, 4 } }, pl_weapon3_alt_steps_sweep_0, 1, 1, 22 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x485, { 0, 0 }, 4, 4 } }, pl_weapon3_alt_steps_sweep_2, 1, 1, 22 },
    { { { 0xFFFF, { 0, 0 }, 0, 0 }, { 0x485, { 0, 0 }, 4, 4 } }, pl_weapon3_alt_steps_sweep_3, 1, 1, 22 },
};

/* The weapon-3 hit-parameter list (`.sdata`) and the row tables per form the row acts index. */
u32* pl_weapon3_hit_ids[2] = { pl_weapon3_hit_params_a, pl_weapon3_hit_params_b };
PlWeapon3Row* pl_weapon3_combo_forms[2] = { pl_weapon3_rows_combo, pl_weapon3_alt_rows_combo };
PlWeapon3Row* pl_weapon3_thrust_forms[2] = { pl_weapon3_rows_thrust, pl_weapon3_alt_rows_thrust };
PlWeapon3Row* pl_weapon3_finish_forms[2] = { pl_weapon3_rows_finish, pl_weapon3_alt_rows_finish };
PlWeapon3Row* pl_weapon3_sweep_forms[2] = { pl_weapon3_rows_sweep, pl_weapon3_alt_rows_sweep };

/* 0x8034D2B0 (0xD0): builds the actor's weapon-3 attack entry from a signed attack value (the sign sets the flag
 * bit 16); `kind` picks the entry's flag set.  Clears the first-hit latch. */
void pl_weapon3_attack_set(_PLW* self, s16 value, u8 kind)
{
    u16 magnitude;
    u16 flags;

    if (value < 0) {
        magnitude = (u16)-value;
        flags = 16;
    } else {
        magnitude = (u16)value;
        flags = 0;
    }
    switch (kind) {
    case 0:
        pl_hit_attack_build(self, &self->hit_0x484, (u8*)pl_weapon3_hit_table, magnitude, (s32*)pl_weapon3_hit_ids,
                    (u16)(flags | 7));
        break;
    case 1:
        pl_hit_attack_build(self, &self->hit_0x484, (u8*)pl_weapon3_hit_table, magnitude, (s32*)pl_weapon3_hit_ids,
                    (u16)(flags | 5));
        break;
    case 2:
        pl_hit_attack_build(self, &self->hit_0x484, (u8*)pl_weapon3_hit_table, magnitude, (s32*)pl_weapon3_hit_ids,
                    (u16)(flags | 0x87));
        break;
    }
    self->field_0x565 = 0;
}

/* 0x8034D380 (0xD4): latches the first landed hit of a swing; a bounced hit (outside the two shell acts) shakes the
 * camera and enters the recoil act.  Returns 1 when it entered the recoil. */
u32 pl_weapon3_hit_bounce_ck(_PLW* self)
{
    u32 shell_act;

    if (self->field_0x565 == 0) {
        shell_act = 0;
        switch (self->act_no) {
        case 40:
        case 50:
            shell_act = 1;
            break;
        }
        switch (pl_hit_result_best_get(self)) {
        case 0:
        case 1:
        case 5:
            self->field_0x565 = 1;
            if (shell_act != 1) {
                camera_pl_quake_req(self, 0);
                if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 4, 0x1A, 0x4080);
                } else {
                    pl_act_enter(self, 4, 0xF, 0x4000);
                }
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x8034D454 (0xC8): applies one half of an act row (the first half in phase 0 when the row has one, else the
 * second): its motion, and the step table and attack the row tags with that half.  Returns the next act step. */
u8 pl_weapon3_row_apply(_PLW* self, PlWeapon3Row* row, u8 phase)
{
    u8 next;
    u32 half;
    PlWeapon3RowHalf* entry;

    if (phase == 0 && row->half[0].motion != 0xFFFF) {
        half = 0;
        next = 1;
    } else {
        half = 1;
        next = 2;
    }
    entry = &row->half[half];
    Pl_chr_set_attr_default(self, entry->motion, entry->arg_a, entry->arg_b);
    if (row->step_half == half) {
        Pl_act_set_step_table(self, (u32)row->step_table, 0);
    }
    if (row->attack_half == half) {
        pl_weapon3_attack_set(self, row->attack, 0);
    }
    return next;
}

/* 0x8034D51C (0x1A8): the combo act: plays the form's combo row `row` in two halves (row 4 steps the act timer, row 5
 * charges the sub gauge), lets a bounce interrupt it and returns to the form's idle slot. */
void pl_weapon3_act_combo(_PLW* self, s32 row, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        self->field_0x018 = 1;
        pl_act_clear_flag5bb(self);
        if (form == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        if (row == 4) {
            pl_act_timer_step(self);
        } else if (row == 5) {
            pl_act_arm_flags(self, 1);
            pl_sub_gauge_level_cost(self);
        }
        self->act_step_0x05 = pl_weapon3_row_apply(self, &pl_weapon3_combo_forms[form][row], 0);
        break;
    case 1:
        if ((row != 1 || pl_weapon3_hit_bounce_ck(self) != 1) && Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon3_row_apply(self, &pl_weapon3_combo_forms[form][row], 1);
        }
        break;
    case 2:
        if (pl_weapon3_hit_bounce_ck(self) != 1 && Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
        }
        break;
    }
}

/* 0x8034D6C4 (0x78): plays the impact sound at the actor's impact position: the plain shell sound, or the charged
 * hit sound. */
void pl_weapon3_impact_se_req(_PLW* self, u8 charged)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    pl_impact_pos_get(self, &pos);
    if (charged == 0) {
        shell_se_req(self->field_0xAFC, &pos, 21, self->chunk_ofs);
    } else {
        pl_se_req_charge_hit(self, &pos);
    }
}

/* 0x8034D73C (0x64): 1 when either of the actor's two attack hit records carries hit mask 2. */
u32 pl_weapon3_hit_mask_ck(_PLW* self)
{
    PlHitSlot* slot;
    s32 i;

    i = 0;
    slot = (PlHitSlot*)self->hit_0x484;
    do {
        if (hit_mask_ck((struct _HIT_W*)slot, 2) == 1) {
            return 1;
        }
        slot++;
        i++;
    } while (i < 2);
    return 0;
}

/* 0x8034D7A0 (0x28): enters the form's charge-stop act. */
void pl_weapon3_charge_stop_enter(_PLW* self, s32 form)
{
    if (form == 0) {
        pl_act_enter(self, 4, 13, 0);
    } else {
        pl_act_enter(self, 4, 33, 0x80);
    }
}

/* 0x8034D7C8 (0x154): reacts to a charge's landed hit: a hit-mask hit stops the charge, otherwise (without skill
 * 0x18) the armed weapon class picks the follow-up act.  Returns 1 when it entered an act. */
u32 pl_weapon3_charge_hit_ck(_PLW* self, s32 form)
{
    if (pl_hit_result_best_get(self) != 0xFF) {
        if (pl_weapon3_hit_mask_ck(self) == 1) {
            pl_weapon3_charge_stop_enter(self, form);
            return 1;
        }
        if (Pl_Skill_ck(self, 0x18) == 0) {
            switch (self->field_0x56B) {
            case 0:
                if (form == 0) {
                    pl_act_enter_raw(self, 6, 2, 0);
                } else {
                    pl_act_enter_raw(self, 6, 8, 0x80);
                }
                return 1;
            case 1:
                if (form == 0) {
                    pl_act_enter(self, 4, 14, 0);
                } else {
                    pl_act_enter(self, 4, 30, 0x80);
                }
                return 1;
            case 2:
                if (form == 0) {
                    pl_act_enter(self, 4, 12, 0);
                } else {
                    pl_act_enter(self, 4, 32, 0x80);
                }
                return 1;
            }
        }
    }
    return 0;
}

/* 0x8034D91C (0x40C): the charge: the wind-up (skipped for `start` 1), then the run that drains the gauge, re-arms
 * its attack whenever the re-hit timer runs out, steers with the stick and ends on the stop or a hit. */
void pl_weapon3_act_charge(_PLW* self, s32 start, s32 form)
{
    pl_act_set_motion_gate(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        if (start == 0) {
            self->act_step_0x05++;
            if (form == 0) {
                Pl_chr_set_attr_default(self, 0x44D, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x47F, 4, 0);
                pl_weapon3_attack_set(self, 9, 2);
            }
        } else {
            self->act_step_0x05 = 2;
            if (form == 0) {
                Pl_chr_set_attr_default(self, 0x44E, 0, 6);
                Pl_act_set_step_table(self, (u32)pl_weapon3_steps_charge, 0);
                pl_weapon3_attack_set(self, 7, 2);
            } else {
                Pl_chr_set_attr_default(self, 0x480, 0, 0);
                Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_charge, 0);
                pl_weapon3_attack_set(self, 20, 2);
            }
            self->rehit_timer_0x2C = 13.0f;
        }
        if (form == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        pl_act_arm_flags(self, 1);
        pl_act_gauge_gate(self, -2);
        self->field_0x006 = 0;
        break;
    case 1:
        if (Pl_frame_check(self, 1, 50.0f, 0.0f) == 1) {
            pl_act_gauge_gate(self, -2);
        }
        if (pl_weapon3_charge_hit_ck(self, form) != 1 && Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (form == 0) {
                Pl_chr_set_attr_default(self, 0x44E, 0, 4);
                Pl_act_set_step_table(self, (u32)pl_weapon3_steps_charge, 0);
                pl_weapon3_attack_set(self, 7, 2);
            } else {
                Pl_chr_set_attr_default(self, 0x480, 0, 4);
                Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_charge, 0);
                pl_weapon3_attack_set(self, 20, 2);
            }
            self->rehit_timer_0x2C = 13.0f;
        }
        break;
    case 2:
        pl_act_gauge_gate(self, -2);
        pl_weapon3_impact_se_req(self, 0);
        self->rehit_timer_0x2C -= self->field_0x354;
        if (self->rehit_timer_0x2C <= 0.0f) {
            if (form == 0) {
                pl_weapon3_attack_set(self, 7, 2);
            } else {
                pl_weapon3_attack_set(self, 20, 2);
            }
            self->rehit_timer_0x2C = 13.0f;
        }
        if (Pl_motion_end_ck(self) == 1 && self->field_0x007 == 0) {
            self->field_0x007++;
            self->field_0x354 = 1.2f;
            eft029_set_scale(self, 22, 0.5f);
        }
        if (Pl_master_ck(self) == 1 && pl_act_param_tier_ck(self, 0) >= 1) {
            if (pl_stick_dir_ck(self, 0x800) == 1) {
                self->field_0x0A8 += 0x80;
            } else if (pl_stick_dir_ck(self, 0x400) == 1) {
                self->field_0x0A8 -= 0x80;
            }
        }
        if (self->field_0x378 <= 0) {
            if (form == 0) {
                pl_act_enter_raw(self, 4, 5, 4);
            } else {
                pl_act_enter_raw(self, 4, 29, 0x84);
            }
        } else if (pl_weapon3_charge_hit_ck(self, form) == 0 && (self->field_0x364 & 0xE0000007) != 0) {
            pl_weapon3_charge_stop_enter(self, form);
        }
        break;
    }
}

/* 0x8034DD28 (0x16C): the recoil: the form's recoil motion (`kind` 1 the heavy one) with its impact sound, then
 * back to the form's idle slot. */
void pl_weapon3_act_recoil(_PLW* self, s32 kind, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        pl_act_set_motion_gate(self, 0);
        if (form == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
        }
        if (kind == 0) {
            if (form == 0) {
                Pl_chr_set_attr_default(self, 0x44F, 2, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x481, 2, 0);
            }
        } else if (form == 0) {
            Pl_chr_set_attr_default(self, 0x457, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x489, 0, 0);
        }
        pl_weapon3_impact_se_req(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
        }
        break;
    }
}

/* 0x8034DE94 (0x1CC): the guard swing: the form's swing with its attack under a 60-frame motion gate, an impact
 * sound on the frame window and once more afterwards, then back to the form's idle slot. */
void pl_weapon3_act_guard_swing(_PLW* self, s32 kind, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (form == 0) {
            Pl_chr_set_attr_default(self, 0x458, 2, 0);
            Pl_act_set_motion(self, 0, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_steps_guard_swing, 0);
            pl_weapon3_attack_set(self, 8, 2);
        } else {
            if (kind == 0) {
                Pl_chr_set_attr_default(self, 0x48A, 2, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x48A, 6, 12);
            }
            Pl_act_set_motion(self, 3, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_guard_swing, 0);
            pl_weapon3_attack_set(self, 10, 2);
        }
        pl_act_set_motion_gate(self, 60);
        pl_weapon3_impact_se_req(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_set_motion_gate(self, 0);
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
        } else if (Pl_frame_check(self, 2, 50.0f, 0.0f) == 1) {
            pl_weapon3_impact_se_req(self, 0);
        } else if (self->field_0x006 == 0) {
            self->field_0x006++;
            pl_weapon3_impact_se_req(self, 1);
        }
        break;
    }
}

/* 0x8034E060 (0x170): the thrust act: plays the form's thrust row `row` in two halves (row 4 also charges the sub
 * gauge), lets a bounce interrupt it and returns to the form's idle slot. */
void pl_weapon3_act_thrust(_PLW* self, s32 row, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        if (form == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        self->act_step_0x05 = pl_weapon3_row_apply(self, &pl_weapon3_thrust_forms[form][row], 0);
        if (row == 4) {
            pl_act_arm_flags(self, 1);
            pl_sub_gauge_level_cost(self);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon3_row_apply(self, &pl_weapon3_thrust_forms[form][row], 1);
        }
        break;
    case 2:
        if (pl_weapon3_hit_bounce_ck(self) != 1 && Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
        }
        break;
    }
}

/* 0x8034E1D0 (0xEC): the finishing swing: the form's single finish row, then the form's follow-up act. */
void pl_weapon3_act_finish(_PLW* self, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (form == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        pl_weapon3_row_apply(self, pl_weapon3_finish_forms[form], 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                pl_act_enter(self, 5, 1, 0);
            } else {
                pl_act_enter(self, 5, 7, 0x80);
            }
        } else if (pl_weapon3_hit_bounce_ck(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x8034E2BC (0x198): the back step: the form's step motion with its impact sound, then the form's guard act
 * (`kind` picks which). */
void pl_weapon3_act_step_back(_PLW* self, s32 kind, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (form == 0) {
            Pl_chr_set_attr_default(self, 0x4C6, 0, 0);
            Pl_act_set_motion(self, 0, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_steps_step_back, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x4F8, 0, 0);
            Pl_act_set_motion(self, 3, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_step_back, 0);
        }
        pl_weapon3_impact_se_req(self, 1);
        pl_act_set_motion_gate(self, 0);
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        pl_act_gauge_gate(self, -2);
        if (Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                if (kind == 0) {
                    pl_act_enter(self, 4, 4, 0);
                } else {
                    pl_act_enter(self, 4, 5, 0);
                }
            } else if (kind == 0) {
                pl_act_enter(self, 4, 28, 0x80);
            } else {
                pl_act_enter(self, 4, 29, 0x80);
            }
        }
        break;
    }
}

/* 0x8034E454 (0x120): the ground slam: the form's slam motion with a camera quake, then the form's idle slot. */
void pl_weapon3_act_quake(_PLW* self, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (form == 0) {
            Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_steps_quake, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_quake, 0);
        }
        camera_pl_quake_req(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 4, 0);
            }
        }
        break;
    }
}

/* 0x8034E574 (0x150): the hold: counts the held frames; a release in the early window enters the quick release, a
 * hold of 60 frames past the late window the full one. */
void pl_weapon3_act_hold(_PLW* self, s32 form)
{
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x28 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_flag(self, 0x12);
        Pl_chr_set_attr_default(self, 0x460, 4, 0);
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if ((u32)pl_part_flag2_ck(self, 11) == 1 && self->field_0x006 == 0) {
                self->field_0x006 = 1;
            }
            if (Pl_frame_check(self, 0, 40.0f, 0.0f) == 1 && self->field_0x006 != 0) {
                pl_act_enter(self, 4, 0x24, 0xC);
            } else if (Pl_frame_check(self, 1, 42.0f, 0.0f) == 1 && self->field_0x28 >= 60) {
                pl_act_enter(self, 4, 0x23, 0xC);
            }
        }
        break;
    }
}

/* 0x8034E6C4 (0xD8): the hold's release swing with its attack, then the idle slot. */
void pl_weapon3_act_hold_release(_PLW* self, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x461, 2, 0);
        pl_weapon3_attack_set(self, 12, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon3_steps_hold_release, 0);
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        if (pl_weapon3_hit_bounce_ck(self) != 1 && Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8034E79C (0x154): the dash: charges the sub gauge 150 under a 100-frame motion gate, then the form's follow-up
 * act. */
void pl_weapon3_act_dash(_PLW* self, s32 form)
{
    pl_act_set_motion_gate(self, 100);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        pl_act_set_flag(self, 0x2010);
        pl_act_arm_flags(self, 1);
        pl_sub_gauge_skill_add(self, -150);
        if (form == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x45D, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_steps_dash, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x48F, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_dash, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                pl_act_enter(self, 5, 1, 0);
            } else {
                pl_act_enter(self, 5, 7, 0x80);
            }
        }
        break;
    }
}

/* 0x8034E8F0 (0xCC): the shell act: the shell motion with its shell ids and attack, then the idle slot. */
void pl_weapon3_act_shell(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x45E, 4, 0);
        pl_shell_ids_set(self, 0, 100);
        pl_weapon3_attack_set(self, 14, 1);
        Pl_act_set_step_table(self, (u32)pl_weapon3_steps_shell, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8034E9BC (0x168): the sweep act: plays the form's sweep row `row` in two halves under a 50-frame motion gate (the
 * second half reads the thrust table), lets a bounce interrupt it and returns to the form's idle slot. */
void pl_weapon3_act_sweep(_PLW* self, s32 row, s32 form)
{
    pl_act_set_motion_gate(self, 50);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (form == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        self->act_step_0x05 = pl_weapon3_row_apply(self, &pl_weapon3_sweep_forms[form][row], 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon3_row_apply(self, &pl_weapon3_thrust_forms[form][row], 1);
        }
        break;
    case 2:
        if (pl_weapon3_hit_bounce_ck(self) != 1 && Pl_motion_end_ck(self) == 1) {
            if (form == 0) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 4, 0);
            }
        }
        break;
    }
}

/* 0x8034EB24 (0xCC): the alternate form's shell act: the shell motion with its shell ids and attack, then the idle
 * slot. */
void pl_weapon3_alt_act_shell(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x490, 4, 0);
        pl_shell_ids_set(self, 0, 100);
        pl_weapon3_attack_set(self, 23, 1);
        Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_shell, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8034EBF0 (0x150): the alternate form's hold: counts the held frames; a release in the early window enters the
 * quick release, a hold of 60 frames past the late window the full one. */
void pl_weapon3_alt_act_hold(_PLW* self, s32 form)
{
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x28 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_flag(self, 0x12);
        Pl_chr_set_attr_default(self, 0x492, 4, 0);
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if ((u32)pl_part_flag2_ck(self, 11) == 1 && self->field_0x006 == 0) {
                self->field_0x006 = 1;
            }
            if (Pl_frame_check(self, 0, 40.0f, 0.0f) == 1 && self->field_0x006 != 0) {
                pl_act_enter(self, 4, 0x34, 0x8C);
            } else if (Pl_frame_check(self, 1, 42.0f, 0.0f) == 1 && self->field_0x28 >= 60) {
                pl_act_enter(self, 4, 0x2D, 0x8C);
            }
        }
        break;
    }
}

/* 0x8034ED40 (0xD8): the alternate form's hold release swing with its attack, then the idle slot. */
void pl_weapon3_alt_act_hold_release(_PLW* self, s32 form)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x493, 2, 0);
        pl_weapon3_attack_set(self, 21, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon3_alt_steps_hold_release, 0);
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        if (pl_weapon3_hit_bounce_ck(self) != 1 && Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8034EE18 (0x320): the weapon-3 act dispatcher: the actor's act number picks its handler and its (row, form)
 * arguments; an act number outside the table does nothing. */
void pl_weapon3_act_dispatch(_PLW* self)
{
    switch (self->act_no) {
    case 0:
        pl_weapon3_act_combo(self, 0, 0);
        break;
    case 1:
        pl_weapon3_act_combo(self, 2, 0);
        break;
    case 2:
        pl_weapon3_act_combo(self, 3, 0);
        break;
    case 3:
        pl_weapon3_act_charge(self, 0, 0);
        break;
    case 4:
        pl_weapon3_act_charge(self, 1, 0);
        break;
    case 5:
        pl_weapon3_act_recoil(self, 0, 0);
        break;
    case 6:
        pl_weapon3_act_guard_swing(self, 0, 0);
        break;
    case 7:
        pl_weapon3_act_thrust(self, 0, 0);
        break;
    case 8:
        pl_weapon3_act_thrust(self, 1, 0);
        break;
    case 9:
        pl_weapon3_act_thrust(self, 2, 0);
        break;
    case 10:
        pl_weapon3_act_finish(self, 0);
        break;
    case 11:
        pl_weapon3_act_combo(self, 1, 0);
        break;
    case 12:
        pl_weapon3_act_step_back(self, 0, 0);
        break;
    case 13:
        pl_weapon3_act_step_back(self, 1, 0);
        break;
    case 14:
        pl_weapon3_act_recoil(self, 1, 0);
        break;
    case 15:
        pl_weapon3_act_quake(self, 0);
        break;
    case 16:
        pl_weapon3_act_combo(self, 4, 0);
        break;
    case 17:
        pl_weapon3_act_combo(self, 0, 1);
        break;
    case 19:
        pl_weapon3_act_combo(self, 2, 1);
        break;
    case 20:
        pl_weapon3_act_combo(self, 3, 1);
        break;
    case 18:
        pl_weapon3_act_combo(self, 1, 1);
        break;
    case 21:
        pl_weapon3_act_combo(self, 4, 1);
        break;
    case 22:
        pl_weapon3_act_thrust(self, 0, 1);
        break;
    case 23:
        pl_weapon3_act_thrust(self, 1, 1);
        break;
    case 24:
        pl_weapon3_act_thrust(self, 2, 1);
        break;
    case 25:
        pl_weapon3_act_finish(self, 1);
        break;
    case 26:
        pl_weapon3_act_quake(self, 1);
        break;
    case 27:
        pl_weapon3_act_charge(self, 0, 1);
        break;
    case 28:
        pl_weapon3_act_charge(self, 1, 1);
        break;
    case 29:
        pl_weapon3_act_recoil(self, 0, 1);
        break;
    case 30:
        pl_weapon3_act_recoil(self, 1, 1);
        break;
    case 31:
        pl_weapon3_act_guard_swing(self, 0, 1);
        break;
    case 32:
        pl_weapon3_act_step_back(self, 0, 1);
        break;
    case 33:
        pl_weapon3_act_step_back(self, 1, 1);
        break;
    case 34:
        pl_weapon3_act_hold(self, 0);
        break;
    case 35:
        pl_weapon3_act_hold_release(self, 0);
        break;
    case 36:
        pl_weapon3_act_thrust(self, 3, 0);
        break;
    case 37:
        pl_weapon3_act_thrust(self, 4, 0);
        break;
    case 38:
        pl_weapon3_act_combo(self, 5, 0);
        break;
    case 39:
        pl_weapon3_act_dash(self, 0);
        break;
    case 40:
        pl_weapon3_act_shell(self);
        break;
    case 41:
        pl_weapon3_act_combo(self, 6, 0);
        break;
    case 42:
        pl_weapon3_act_sweep(self, 0, 0);
        break;
    case 43:
        pl_weapon3_act_sweep(self, 0, 1);
        break;
    case 44:
        pl_weapon3_alt_act_hold(self, 0);
        break;
    case 45:
        pl_weapon3_alt_act_hold_release(self, 0);
        break;
    case 46:
        pl_weapon3_act_thrust(self, 3, 1);
        break;
    case 47:
        pl_weapon3_act_thrust(self, 4, 1);
        break;
    case 48:
        pl_weapon3_act_combo(self, 5, 1);
        break;
    case 49:
        pl_weapon3_act_dash(self, 1);
        break;
    case 50:
        pl_weapon3_alt_act_shell(self);
        break;
    case 51:
        pl_weapon3_act_combo(self, 6, 1);
        break;
    case 52:
        pl_weapon3_act_combo(self, 7, 1);
        break;
    case 53:
        pl_weapon3_act_combo(self, 8, 0);
        break;
    case 54:
        pl_weapon3_act_combo(self, 8, 1);
        break;
    case 55:
        pl_weapon3_act_thrust(self, 5, 0);
        break;
    case 56:
        pl_weapon3_act_thrust(self, 5, 1);
        break;
    case 57:
        pl_weapon3_act_guard_swing(self, 1, 1);
        break;
    case 58:
        pl_weapon3_act_sweep(self, 1, 0);
        break;
    case 59:
        pl_weapon3_act_sweep(self, 2, 0);
        break;
    case 60:
        pl_weapon3_act_sweep(self, 3, 0);
        break;
    case 61:
        pl_weapon3_act_sweep(self, 1, 1);
        break;
    case 62:
        pl_weapon3_act_sweep(self, 2, 1);
        break;
    case 63:
        pl_weapon3_act_sweep(self, 3, 1);
        break;
    case 66:
        pl_weapon3_act_thrust(self, 6, 0);
        break;
    case 67:
        pl_weapon3_act_thrust(self, 6, 1);
        break;
    case 64:
        pl_weapon3_act_combo(self, 9, 0);
        break;
    case 65:
        pl_weapon3_act_combo(self, 9, 1);
        break;
    }
}

/* The act step tables, emitted after the dispatcher's jump table. */
u32 pl_weapon3_steps_combo_5[44] = {
    0x04000008, 0x00000019, 0x00060000, 0x00000000,
    0x0024003E, 0x00000400, 0x003B0000, 0x0019000D,
    0x00000000, 0x00000024, 0x003E0000, 0x04000001,
    0x00000019, 0x00010000, 0x00000000, 0x0024003E,
    0x00000000, 0x008B0000, 0x8016003D, 0x00000000,
    0x00010024, 0x00000000, 0x00000010, 0x00008016,
    0x003F0000, 0x00000001, 0x00240000, 0x00000000,
    0x000F0000, 0x8016003E, 0x00000000, 0x00010024,
    0x00000000, 0x0000007B, 0x00008016, 0x00040000,
    0x00000001, 0x00240000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_combo_0[55] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x04000008,
    0x00000019, 0x00060000, 0x00000000, 0x0024003E,
    0x00000400, 0x003B0000, 0x0019000D, 0x00000000,
    0x00000024, 0x003E0000, 0x04000001, 0x00000019,
    0x00010000, 0x00000000, 0x0024003E, 0x00000000,
    0x008B0000, 0x8016003D, 0x00000000, 0x00010024,
    0x00000000, 0x00000010, 0x00008016, 0x003F0000,
    0x00000001, 0x00240000, 0x00000000, 0x000F0000,
    0x8016003E, 0x00000000, 0x00010024, 0x00000000,
    0x0000007B, 0x00008016, 0x00040000, 0x00000001,
    0x00240000, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_combo_2[55] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x04000009,
    0x00000019, 0x00060000, 0x00000000, 0x0024003E,
    0x00000400, 0x003C0000, 0x0019000D, 0x00000000,
    0x00000024, 0x003E0000, 0x04000002, 0x00000019,
    0x00010000, 0x00000000, 0x0024003E, 0x00000000,
    0x008B0000, 0x8016003D, 0x00000000, 0x00010024,
    0x00000000, 0x00000010, 0x00008016, 0x003F0000,
    0x00000001, 0x00240000, 0x00000000, 0x000F0000,
    0x8016003E, 0x00000000, 0x00010024, 0x00000000,
    0x0000007B, 0x00008016, 0x00040000, 0x00000001,
    0x00240000, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_combo_3[39] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x0000008B,
    0x00008016, 0x003D0000, 0x00000001, 0x00240000,
    0x00000000, 0x00100000, 0x8016003F, 0x00000000,
    0x00010024, 0x00000000, 0x0000000F, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x00000000,
    0x007B0000, 0x80160004, 0x00000000, 0x00010024,
    0x00000000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_charge[17] = {
    0x04000006, 0x00000019, 0x00040000, 0x00000000,
    0x00000000, 0x00000400, 0x00050000, 0x00190003,
    0x00000000, 0x00000000, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_steps_guard_swing[50] = {
    0x04000007, 0x00000019, 0x00060000, 0x00000000,
    0x00480066, 0x00000400, 0x003A0000, 0x0019000D,
    0x00000000, 0x00000048, 0x00660000, 0x04000000,
    0x00000019, 0x00010000, 0x00000000, 0x00480066,
    0x00000400, 0x00030000, 0x00190002, 0x00000000,
    0x00000048, 0x00660000, 0x0000008B, 0x00008016,
    0x003D0000, 0x00000001, 0x00480000, 0x00000000,
    0x00100000, 0x8016003F, 0x00000000, 0x00010048,
    0x00000000, 0x0000000F, 0x00008016, 0x003E0000,
    0x00000001, 0x00480000, 0x00000000, 0x007B0000,
    0x80160004, 0x00000000, 0x00010048, 0x00000000,
    0xFF000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_step_back[17] = {
    0x04000006, 0x00000019, 0x00040000, 0x00000000,
    0x00540056, 0x00540400, 0x00050000, 0x00190003,
    0x00000000, 0x00000054, 0x00560054, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_steps_hold_release[39] = {
    0x04000008, 0x00000019, 0x00060000, 0x00000000,
    0x0036004A, 0x00000400, 0x00010000, 0x00190001,
    0x00000000, 0x00000036, 0x004A0000, 0x0000008B,
    0x00008016, 0x003D0000, 0x00000001, 0x00360000,
    0x00000000, 0x00100000, 0x8016003F, 0x00000000,
    0x00010036, 0x00000000, 0x0000000F, 0x00008016,
    0x003E0000, 0x00000001, 0x00360000, 0x00000000,
    0x007B0000, 0x80160004, 0x00000000, 0x00010036,
    0x00000000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_steps_guard_kind3[44] = {
    0x04000022, 0x00000019, 0x00100000, 0x00000000,
    0x00000000, 0x00000400, 0x00270000, 0x001D0007,
    0x00000000, 0x00000000, 0x00000000, 0x04000027,
    0x00000019, 0x000E0000, 0x00000000, 0x00000000,
    0x00000400, 0x000A0000, 0x00190007, 0x00000000,
    0x00000000, 0x00000000, 0x04000003, 0x00000019,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x007B0000, 0x80160033, 0x00000000, 0x00010000,
    0x00000000, 0x0500000D, 0x00008017, 0x00110000,
    0x00000002, 0x00000000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_steps_aim_kind3[44] = {
    0x04000022, 0x00000019, 0x00100000, 0x00000000,
    0x00000000, 0x00000400, 0x00270000, 0x001D0007,
    0x00000000, 0x00000000, 0x00000000, 0x04000027,
    0x00000019, 0x000E0000, 0x00000000, 0x00000000,
    0x00000400, 0x000A0000, 0x00190007, 0x00000000,
    0x00000000, 0x00000000, 0x04000003, 0x00000019,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x007B0000, 0x80160033, 0x00000000, 0x00010000,
    0x00000000, 0x0500000D, 0x00008017, 0x00110000,
    0x00000002, 0x00000000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_dash[11] = {
    0x04000028, 0x00000019, 0x000F0000, 0x00000000,
    0x00240024, 0x0014FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_shell[11] = {
    0x04000029, 0x00000019, 0x00120000, 0x00000000,
    0x00200034, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_finish_0[33] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x0012001C, 0x00000000, 0x008B0000, 0x8016003D,
    0x00000000, 0x00010012, 0x00000000, 0x00000010,
    0x00008016, 0x003F0000, 0x00000001, 0x00120000,
    0x00000000, 0x000F0000, 0x8016003E, 0x00000000,
    0x00010012, 0x00000000, 0x0000007B, 0x00008016,
    0x00040000, 0x00000001, 0x00120000, 0x0000FF00,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_steps_sweep_0[55] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x04000037,
    0x00000019, 0x00060000, 0x00000000, 0x003C0050,
    0x00000400, 0x00350000, 0x00190001, 0x00000000,
    0x0000003C, 0x00500000, 0x04000035, 0x00000019,
    0x00110000, 0x00000000, 0x003C0050, 0x00000000,
    0x008B0000, 0x8016003D, 0x00000000, 0x0001003C,
    0x00500000, 0x00000010, 0x00008016, 0x003F0000,
    0x00000001, 0x003C0050, 0x00000000, 0x000F0000,
    0x8016003E, 0x00000000, 0x0001003C, 0x00500000,
    0x0000007B, 0x00008016, 0x00040000, 0x00000001,
    0x003C0050, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_sweep_2[55] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x04000042,
    0x00000019, 0x00060000, 0x00000000, 0x003C0050,
    0x00000400, 0x00400000, 0x00190001, 0x00000000,
    0x0000003C, 0x00500000, 0x04000040, 0x00000019,
    0x00110000, 0x00000000, 0x003C0050, 0x00000000,
    0x008B0000, 0x8016003D, 0x00000000, 0x0001003C,
    0x00500000, 0x00000010, 0x00008016, 0x003F0000,
    0x00000001, 0x003C0050, 0x00000000, 0x000F0000,
    0x8016003E, 0x00000000, 0x0001003C, 0x00500000,
    0x0000007B, 0x00008016, 0x00040000, 0x00000001,
    0x003C0050, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_sweep_3[39] = {
    0x04000003, 0x00000019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x00220000, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x0000008B,
    0x00008016, 0x003D0000, 0x00000001, 0x003C0050,
    0x00000000, 0x00100000, 0x8016003F, 0x00000000,
    0x0001003C, 0x00500000, 0x0000000F, 0x00008016,
    0x003E0000, 0x00000001, 0x003C0050, 0x00000000,
    0x007B0000, 0x80160004, 0x00000000, 0x0001003C,
    0x00500000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_quake[11] = {
    0x04000010, 0x40000019, 0x00080000, 0x00000000,
    0x002A002A, 0x001EFF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_2[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00810000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000080, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00820000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_3[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00840000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000083, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00850000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_0[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00810000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000080, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00820000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_1[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00840000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000083, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00850000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_knockback_steps_bow_alt[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00810000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000080, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00820000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_4[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00810000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000080, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00820000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_knockback_steps_5[39] = {
    0x04000007, 0x10000019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002A1000, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000000,
    0x10000019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00840000, 0x8016003F, 0x00000000,
    0x00010024, 0x0000000A, 0x00000083, 0x00008016,
    0x003E0000, 0x00000001, 0x00240000, 0x000A0000,
    0x00850000, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_combo_5[50] = {
    0x04000017, 0x00800019, 0x00060000, 0x00000000,
    0x0024003E, 0x00000400, 0x003E0080, 0x0019000D,
    0x00000000, 0x00000024, 0x003E0000, 0x04000013,
    0x00800019, 0x00010000, 0x00000000, 0x0024003E,
    0x00000000, 0x00650080, 0x80160041, 0x00000000,
    0x00010024, 0x00000000, 0x00000064, 0x00808016,
    0x00400000, 0x00000017, 0x00240000, 0x00000000,
    0x008C0080, 0x80160044, 0x00000000, 0x00010024,
    0x00000000, 0x0000008D, 0x00808016, 0x00450000,
    0x00000001, 0x00240000, 0x00000000, 0x008E0080,
    0x80160004, 0x00000000, 0x00010024, 0x00000000,
    0xFF000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_combo_0[61] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x04000017,
    0x00800019, 0x00060000, 0x00000000, 0x0024003E,
    0x00000400, 0x003E0080, 0x0019000D, 0x00000000,
    0x00000024, 0x003E0000, 0x04000013, 0x00800019,
    0x00010000, 0x00000000, 0x0024003E, 0x00000000,
    0x00650080, 0x80160041, 0x00000000, 0x00010024,
    0x00000000, 0x00000064, 0x00808016, 0x00400000,
    0x00000017, 0x00240000, 0x00000000, 0x008C0080,
    0x80160044, 0x00000000, 0x00010024, 0x00000000,
    0x0000008D, 0x00808016, 0x00450000, 0x00000001,
    0x00240000, 0x00000000, 0x008E0080, 0x80160004,
    0x00000000, 0x00010024, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_alt_steps_combo_2[61] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x04000018,
    0x00800019, 0x00060000, 0x00000000, 0x0024003E,
    0x00000400, 0x003F0080, 0x0019000D, 0x00000000,
    0x00000024, 0x003E0000, 0x04000014, 0x00800019,
    0x00010000, 0x00000000, 0x0024003E, 0x00000000,
    0x00650080, 0x80160041, 0x00000000, 0x00010024,
    0x00000000, 0x00000064, 0x00808016, 0x00400000,
    0x00000017, 0x00240000, 0x00000000, 0x008C0080,
    0x80160044, 0x00000000, 0x00010024, 0x00000000,
    0x0000008D, 0x00808016, 0x00450000, 0x00000001,
    0x00240000, 0x00000000, 0x008E0080, 0x80160004,
    0x00000000, 0x00010024, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_alt_steps_combo_3[44] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x0024003E, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x00000024, 0x003E0000, 0x00000065,
    0x00808016, 0x00410000, 0x00000001, 0x00240000,
    0x00000000, 0x00640080, 0x80160040, 0x00000000,
    0x00170024, 0x00000000, 0x0000008C, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x00000000,
    0x008D0080, 0x80160045, 0x00000000, 0x00010024,
    0x00000000, 0x0000008E, 0x00808016, 0x00040000,
    0x00000001, 0x00240000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_charge[17] = {
    0x0400001F, 0x00800019, 0x00040000, 0x00000000,
    0x00000000, 0x00000400, 0x001D0080, 0x00190003,
    0x00000000, 0x00000000, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_alt_steps_guard_swing[55] = {
    0x04000016, 0x00800019, 0x00060000, 0x00000000,
    0x00480066, 0x00000400, 0x003D0080, 0x0019000D,
    0x00000000, 0x00000048, 0x00660000, 0x04000011,
    0x00800019, 0x00010000, 0x00000000, 0x00480066,
    0x00000400, 0x001B0080, 0x00190002, 0x00000000,
    0x00000048, 0x00660000, 0x00000065, 0x00808016,
    0x00410000, 0x00000001, 0x00480000, 0x00000000,
    0x00640080, 0x80160040, 0x00000000, 0x00170048,
    0x00000000, 0x0000008C, 0x00808016, 0x00440000,
    0x00000001, 0x00480000, 0x00000000, 0x008D0080,
    0x80160045, 0x00000000, 0x00010048, 0x00000000,
    0x0000008E, 0x00808016, 0x00040000, 0x00000001,
    0x00480000, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_step_back[18] = {
    0x04000039, 0x00800019, 0x00040000, 0x00000000,
    0x00540056, 0x00540400, 0x001D0080, 0x00190003,
    0x00000000, 0x00000054, 0x00560054, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_hold_release[44] = {
    0x04000017, 0x00800019, 0x00060000, 0x00000000,
    0x0036004A, 0x00000400, 0x00130080, 0x00190001,
    0x00000000, 0x00000036, 0x004A0000, 0x00000065,
    0x00808016, 0x00410000, 0x00000001, 0x00360000,
    0x00000000, 0x00640080, 0x80160040, 0x00000000,
    0x00170036, 0x00000000, 0x0000008C, 0x00808016,
    0x00440000, 0x00000001, 0x00360000, 0x00000000,
    0x008D0080, 0x80160045, 0x00000000, 0x00010036,
    0x00000000, 0x0000008E, 0x00808016, 0x00040000,
    0x00000001, 0x00360000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_steps_guard_walk_kind3[44] = {
    0x0400002C, 0x00800019, 0x00100000, 0x00000000,
    0x00000000, 0x00000400, 0x00310080, 0x001D0007,
    0x00000000, 0x00000000, 0x00000000, 0x04000031,
    0x00800019, 0x000E0000, 0x00000000, 0x00000000,
    0x00000400, 0x00190080, 0x00190007, 0x00000000,
    0x00000000, 0x00000000, 0x0400001B, 0x00800019,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x008E0000, 0x80160033, 0x00000000, 0x00010000,
    0x00000000, 0x0500000F, 0x00808017, 0x00110000,
    0x00000002, 0x00000000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_steps_aim_walk_kind3[44] = {
    0x0400002C, 0x00800019, 0x00100000, 0x00000000,
    0x00000000, 0x00000400, 0x00310080, 0x001D0007,
    0x00000000, 0x00000000, 0x00000000, 0x04000031,
    0x00800019, 0x000E0000, 0x00000000, 0x00000000,
    0x00000400, 0x00190080, 0x00190007, 0x00000000,
    0x00000000, 0x00000000, 0x0400001B, 0x00800019,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x008E0000, 0x80160033, 0x00000000, 0x00010000,
    0x00000000, 0x0500000F, 0x00808017, 0x00110000,
    0x00000002, 0x00000000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_dash[11] = {
    0x04000032, 0x00800019, 0x000F0000, 0x00000000,
    0x00240024, 0x0014FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_shell[11] = {
    0x04000033, 0x00800019, 0x00120000, 0x00000000,
    0x0020003E, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_finish_0[39] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x00160020, 0x00000000, 0x00650080, 0x80160041,
    0x00000000, 0x00010016, 0x00000000, 0x00000064,
    0x00808016, 0x00400000, 0x00000017, 0x00160000,
    0x00000000, 0x008C0080, 0x80160044, 0x00000000,
    0x00010016, 0x00000000, 0x0000008D, 0x00808016,
    0x00450000, 0x00000001, 0x00160000, 0x00000000,
    0x008E0080, 0x80160004, 0x00000000, 0x00010016,
    0x00000000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_sweep_0[61] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x04000038,
    0x00800019, 0x00060000, 0x00000000, 0x003C0050,
    0x00000400, 0x00360080, 0x00190001, 0x00000000,
    0x0000003C, 0x00500000, 0x04000036, 0x00800019,
    0x00110000, 0x00000000, 0x003C0050, 0x00000000,
    0x00650080, 0x80160041, 0x00000000, 0x0001003C,
    0x00000000, 0x00000064, 0x00808016, 0x00400000,
    0x00000017, 0x003C0000, 0x00000000, 0x008C0080,
    0x80160044, 0x00000000, 0x0001003C, 0x00000000,
    0x0000008D, 0x00808016, 0x00450000, 0x00000001,
    0x003C0000, 0x00000000, 0x008E0080, 0x80160004,
    0x00000000, 0x0001003C, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_alt_steps_sweep_2[62] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x04000043,
    0x00800019, 0x00060000, 0x00000000, 0x003C0050,
    0x00000400, 0x00410080, 0x00190001, 0x00000000,
    0x0000003C, 0x00500000, 0x04000041, 0x00800019,
    0x00110000, 0x00000000, 0x003C0050, 0x00000000,
    0x00650080, 0x80160041, 0x00000000, 0x0001003C,
    0x00000000, 0x00000064, 0x00808016, 0x00400000,
    0x00000017, 0x003C0000, 0x00000000, 0x008C0080,
    0x80160044, 0x00000000, 0x0001003C, 0x00000000,
    0x0000008D, 0x00808016, 0x00450000, 0x00000001,
    0x003C0000, 0x00000000, 0x008E0080, 0x80160004,
    0x00000000, 0x0001003C, 0x00000000, 0xFF000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_sweep_3[44] = {
    0x0400001B, 0x00800019, 0x00020000, 0x00000000,
    0x003C0050, 0x00000400, 0x002C0080, 0x0019000A,
    0x00000000, 0x0000003C, 0x00500000, 0x00000065,
    0x00808016, 0x00410000, 0x00000001, 0x003C0000,
    0x00000000, 0x00640080, 0x80160040, 0x00000000,
    0x0017003C, 0x00000000, 0x0000008C, 0x00808016,
    0x00440000, 0x00000001, 0x003C0000, 0x00000000,
    0x008D0080, 0x80160045, 0x00000000, 0x0001003C,
    0x00000000, 0x0000008E, 0x00808016, 0x00040000,
    0x00000001, 0x003C0000, 0x0000FF00, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_alt_steps_quake[11] = {
    0x04000015, 0x40800019, 0x00080000, 0x00000000,
    0x002A002A, 0x001EFF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_2[39] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00900080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x0000008F, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00910080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_3[39] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00930080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x00000092, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00940080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_0[39] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00900080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x0000008F, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00910080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_1[40] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00930080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x00000092, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00940080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
u32 pl_evade_steps_bow_alt[22] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000AFF00, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_4[39] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00900080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x0000008F, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00910080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_evade_steps_5[39] = {
    0x04000016, 0x10800019, 0x00060000, 0x00000000,
    0x00240000, 0x000A0400, 0x002B1080, 0x0019000D,
    0x00000000, 0x00000024, 0x0000000A, 0x04000011,
    0x10800019, 0x00010000, 0x00000000, 0x00240000,
    0x000A0000, 0x00930080, 0x80160045, 0x00000000,
    0x00010024, 0x0000000A, 0x00000092, 0x00808016,
    0x00440000, 0x00000001, 0x00240000, 0x000A0000,
    0x00940080, 0x80160004, 0x00000000, 0x00010024,
    0x0000000A, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};
u32 pl_weapon3_steps_shared_a[121] = {
    0x890000F8, 0x00008000, 0x00000000, 0x0000002F,
    0x00000000, 0x00000600, 0x001B0000, 0x80000000,
    0x00000000, 0x00120000, 0x00000000, 0x0600002B,
    0x00008000, 0x00000000, 0x0000002A, 0x00000000,
    0x00000600, 0x003F0000, 0x80000000, 0x00000000,
    0x002B0000, 0x00000000, 0x00000014, 0x00108000,
    0x00000000, 0x00000027, 0x00000000, 0x00000000,
    0x001E0000, 0x80000003, 0x00000000, 0x00240000,
    0x00000000, 0x000000A6, 0x00008016, 0x00340000,
    0x00000031, 0x00000000, 0x00000000, 0x00B10000,
    0x80160034, 0x00000000, 0x00320000, 0x00000000,
    0x00000004, 0x00000017, 0x000E0000, 0x00000000,
    0x00000000, 0x00000000, 0x00030000, 0x0016000D,
    0x00000000, 0x00000000, 0x00000000, 0x04000022,
    0x00000019, 0x00090000, 0x00000000, 0x00000000,
    0x00008400, 0x00020000, 0x8016001C, 0x00000000,
    0x00000000, 0x00000000, 0x04000003, 0x00000019,
    0x00020000, 0x00000000, 0x00000000, 0x00000400,
    0x00070000, 0x00190005, 0x00000000, 0x00000000,
    0x00000000, 0x0400002A, 0x00000019, 0x000C0000,
    0x00000000, 0x00000000, 0x00000400, 0x00000000,
    0x00190000, 0x00000000, 0x00000000, 0x00000000,
    0x0000008B, 0x00008016, 0x003D0000, 0x00000001,
    0x00000000, 0x00000000, 0x007B0000, 0x80160004,
    0x00000000, 0x00010000, 0x00000000, 0x8B0000FA,
    0x00008000, 0x00000000, 0x0000002E, 0x00000000,
    0x00000100, 0x000A0000, 0xC0090000, 0x00000000,
    0x40240000, 0x00000000, 0x01000000, 0x0000C009,
    0x00000000, 0x00004002, 0x00000000, 0x0000FF00,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};
u32 pl_weapon3_steps_shared_b[99] = {
    0x890000F7, 0x00008000, 0x00000000, 0x0000002F,
    0x00000000, 0x00000600, 0x00230000, 0x80000000,
    0x00000000, 0x00120000, 0x00000000, 0x0000009E,
    0x00108000, 0x00000000, 0x00000027, 0x00000000,
    0x00000400, 0x002C0080, 0x00190009, 0x00000000,
    0x00000000, 0x00000000, 0x84000004, 0x00008016,
    0x001C0000, 0x00000000, 0x00000000, 0x00000400,
    0x001B0080, 0x00190002, 0x00000000, 0x00000000,
    0x00000000, 0x04000016, 0x00800019, 0x00050000,
    0x00000000, 0x00000000, 0x00000400, 0x002B0080,
    0x0019000C, 0x00000000, 0x00000000, 0x00000000,
    0x04000011, 0x00800019, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00530000, 0x0017000E,
    0x00000000, 0x00000000, 0x00000000, 0x0000002E,
    0x00000016, 0x000D0000, 0x00000000, 0x00000000,
    0x00000000, 0x00170000, 0x801C0030, 0x00000000,
    0x00160000, 0x00000000, 0x00000018, 0x0000001C,
    0x00310000, 0x00000000, 0x00000000, 0x00000000,
    0x008E0080, 0x80170004, 0x00000000, 0x00010000,
    0x00000000, 0x0000008E, 0x0080801C, 0x002F0000,
    0x00000001, 0x00000000, 0x00008B00, 0x00F90000,
    0x80000000, 0x00000000, 0x002E0000, 0x00000000,
    0x01000006, 0x0080C009, 0x00000000, 0x0000401C,
    0x00000000, 0x0000FF00, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};

}
