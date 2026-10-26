/*
 * Pl/fn_802840DC.cpp - the player's weapon act handlers for weapon classes 2 and 4-6, their dispatchers, the weapon-1
 *   dispatcher and the class-7 attack-set/motion-row pair.
 * RANGE. .text 0x802840DC-0x80288CEC (54 functions); .data 0x805C9568-0x805CBFC0, .sdata 0x80792188-0x807921B0,
 *   .sdata2 0x8079A1D8-0x8079A270, extab, extabindex.  The range is four TU pieces (seam request pl-802840dc#1):
 *   0x802840DC-0x80284204 ends the weapon-1 TU, 0x80284204-0x80287244 is the weapon-2 TU, 0x80287244-0x80288B98 the
 *   weapon-4/5/6 TU and 0x80288B98-0x80288CEC opens the class-7 TU (`Pl/pl_motion.cpp`).
 * FLAGS. The unit's `cflags`; the weapon-4/5/6 piece is under `#pragma optimization_level 4` (its switches test case
 *   ranges with `subi`+`cmplwi`, playbook 108; every row of that piece is 100 % with it, the weapon-2 rows at -O3).
 * NAMES. No `__FILE__` string covers the range; the file keeps the map's stem (unit-rename request pl-802840dc#2).
 *   GUESS (from each body, its dispatcher case and its callers): every `pl_weapon2_*`, `pl_weapon456_*`,
 *   `pl_act4_phase_ck` (registry), `pl_scan_rows_id252_ck`, `pl_gun_guard_row_latch`, `pl_act_attack_set`, `pl_act_motion_row_apply`,
 *   `pl_charge_stage2_get`, the data rows `pl_weapon2_*`/`pl_weapon456_*`/`pl_class7_hit_*` and the types
 *   `ShellAtkRow`, `PlGunFireMotion`, `GunAmmoEntry`, `PlPhysicsMuzzle`; registry names (GUESS) for the foreign
 *   callees `pl_stick_dir_ck`, `pl_camera_lock_clear`, `pl_aim_motion_set`, `pl_gun_reload_tier_get`,
 *   `pl_gun_ammo_entry_get`, `pl_act_state_enter_aim`, `pl_motion_stick_move`, `pl_gun_shot_se_req`,
 *   `pl_gun_empty_se_req`, `eft042_set`.
 * RESIDUALS. Every row is written.
 *  - `pl_weapon2_act_shell_fire`, `pl_weapon2_alt_act_shell_fire`, `pl_weapon456_act_fire`: retail loads
 *    `shell_set_func_ptr` for a table call after the other arguments, ours first (`lwz r5` scheduled ahead of
 *    `mr r3`/`li r4`); an explicit `(*p->slot)(...)` and an inline wrapper give the same order.  The fire act also
 *    keeps the rapid-shot row index in r4 where ours uses r3.
 *  - Data: the tables are declared, not emitted (`.data` 0x37C of 0x2A58 - our jump tables only, `.sdata` none of
 *    0x28, `.sdata2` 0x80 of 0x98); the run spans the four TU pieces above, so it is claimed after the seam.
 *  - flipcheck: the 10 registry callees have no map row yet; `pl_flinch_steps_kind2`, `pl_scan_rows_guard_a/b`,
 *    `lbl_805CAD74`, `lbl_805CADA0`, `lbl_805CAF2C` are referenced by `Pl/pl_act_step.cpp` and not emitted here.
 *  - `pl_gun_reload_tier_get` is declared returning `u8` (its leaf header): the callers store the result with a raw
 *    `stb r3` and extend it before a compare, while `Pl/pl_act.cpp` defines it returning `s32`.
 * SHAPES. The `.sdata` id lists are sized extern arrays (an unsized one gives `lis`/`addi`, the target `@sda21`).
 *   A shared tail reached by `b` from switch cases is `do { switch ... continue / break ...; tail } while (0)`
 *   (playbook 183: `pl_weapon2_hit_react`, `pl_weapon2_alt_act_charge`).  Narrow sums: `field_0x0B6 += (u16)3`
 *   keeps the raw `sth`.  Retail compares a `u16` result signed (`(s32)Get_motion_no(...)`, an `s32` local) and a
 *   `s32` flag result unsigned (`(u32)pl_part_flag2_ck(...) == 1`).
 */

#include "types.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_802840DC.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "unsplit/Pl.h"
#include "MSL/mathf.h"
#include "Pl/pl_act_gauge_gate.h"
#include "Pl/pl_act_hold_notify.h"
#include "Pl/pl_act4_phase_ck.h"
#include "Pl/pl_act_guard_gauge_adjust.h"
#include "Pl/pl_atk_act_flag_set.h"
#include "Pl/pl_hit_result_best_get.h"
#include "Pl/pl_now_area_ck.h"
#include "Pl/pl_stick_dir_ck.h"
#include "camera/camera_main.h"
#include "ef/eft029_fx.h"
#include "mh3_pad.h"
#include "sound/fn_800D7F54.h"
#include "stage/shell_set_func_ptr.h"
#include "Pl/pl_camera_lock_clear.h"
#include "Pl/pl_aim_motion_set.h"
#include "Pl/pl_gun_reload_tier_get.h"
#include "Pl/pl_gun_ammo_entry_get.h"
#include "Pl/pl_act_state_enter_aim.h"
#include "Pl/pl_motion_stick_move.h"
#include "Pl/pl_weapon_alt_form_ck.h"
#include "Pl/pl_item_slot_item_get.h"
#include "Pl/pl_item_add.h"
#include "menu/eft043_set.h"
#include "sound/pl_gun_shot_se_req.h"
#include "sound/pl_gun_empty_se_req.h"
#include "ef/eft042_set.h"
#include "ef/cp_vector.h"
#include "camera/camera_pl_quake2_req.h"

extern "C" {

/* The actor's current shell act (`_PLW::act_no`, 0-34) dispatched to its per-act handler; an act
 * number outside the table does nothing. */
void pl_weapon1_act_dispatch(_PLW* self)
{
    switch (self->act_no) {
    case 0:
        pl_weapon1_act_combo(self, 0);
        return;
    case 1:
        pl_weapon1_act_combo(self, 1);
        return;
    case 2:
        pl_weapon1_act_combo(self, 2);
        return;
    case 3:
        pl_weapon1_act_combo(self, 3);
        return;
    case 4:
        pl_weapon1_act_shell_swing(self, 0);
        return;
    case 5:
        pl_weapon1_act_shell_swing(self, 1);
        return;
    case 6:
        pl_weapon1_act_double(self, 0);
        return;
    case 7:
        pl_weapon1_act_double(self, 1);
        return;
    case 8:
        pl_weapon1_act_double(self, 2);
        return;
    case 9:
        pl_weapon1_act_turn_swing(self, 0);
        return;
    case 10:
        pl_weapon1_act_turn_swing(self, 1);
        return;
    case 11:
        pl_weapon1_act_turn_swing(self, 2);
        return;
    case 12:
        pl_weapon1_act_swing2(self, 0);
        return;
    case 13:
        pl_weapon1_act_swing2(self, 1);
        return;
    case 14:
        pl_weapon1_act_chain(self);
        return;
    case 15:
        pl_weapon1_act_single(self);
        return;
    case 16:
        pl_weapon1_act_combo3(self, 0);
        return;
    case 17:
        pl_weapon1_act_combo3(self, 1);
        return;
    case 18:
        pl_weapon1_act_combo3(self, 2);
        return;
    case 19:
        pl_weapon1_act_rise(self, 1);
        return;
    case 20:
        pl_weapon1_act_rise(self, 0);
        return;
    case 21:
        pl_weapon1_act_double3(self, 0);
        return;
    case 22:
        pl_weapon1_act_double3(self, 1);
        return;
    case 23:
        pl_weapon1_act_single3(self);
        return;
    case 24:
        pl_weapon1_act_finisher3(self, 0);
        return;
    case 25:
        pl_weapon1_act_chain3(self);
        return;
    case 26:
        pl_weapon1_act_kick3(self);
        return;
    case 27:
        pl_weapon1_act_kick(self);
        return;
    case 28:
        pl_weapon1_act_turn_swing(self, 3);
        return;
    case 29:
        pl_weapon1_act_turn_swing(self, 4);
        return;
    case 30:
        pl_weapon1_act_turn3(self, 0);
        return;
    case 31:
        pl_weapon1_act_turn3(self, 1);
        return;
    case 32:
        pl_weapon1_act_turn3(self, 2);
        return;
    case 33:
        pl_weapon1_act_double3(self, 2);
        return;
    case 34:
        pl_weapon1_act_finisher3(self, 1);
        return;
    }
}

/* Builds the actor's weapon-2 attack entry from a signed attack value: the magnitude is the attack index, the sign,
 * the attack-flag predicate and the caller's flag pick the entry's flag bits; then runs the shared set-up. */
void pl_weapon2_attack_set(_PLW* self, s16 value, u8 flag)
{
    u16 magnitude;
    u16 flags;

    if (value < 0) {
        magnitude = (u16)-value;
        flags = 16;
    } else {
        magnitude = (u16)value;
        flags = (Pl_atk_act_flag_ck(self, 1) == 1) ? 16 : 0;
    }
    if (flag != 0) {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon2_hit_table, magnitude, pl_weapon2_hit_ids,
                    (u16)(flags | 4));
    } else {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon2_hit_table, magnitude, pl_weapon2_hit_ids,
                    (u16)(flags | 7));
    }
}

/* Applies one attack row: picks the field set the kind/mode selects, sets the motion, adds the row's frame
 * offset onto the actor's frame counters and runs the row's attack value and motion gate. */
void pl_weapon2_row_apply(_PLW* self, u8 kind, ShellAtkRow* row, u32 table)
{
    u16 motion;
    u16 add;
    s16 a;
    s16 b;
    s16 attack;

    if (kind == 1 || row->mode == 0xFFFF) {
        motion = row->motion_0x0A;
        a = row->param_0x0C;
        b = row->param_0x0E;
        attack = row->attack_0x10;
        add = 0;
        Pl_act_set_step_table(self, table, 0);
        if (kind == 0) {
            self->act_step_0x05++;
        }
    } else {
        motion = row->mode;
        a = row->value_0x02;
        b = row->value_0x04;
        attack = 0;
        add = row->add_0x06;
    }
    Pl_chr_set_attr_default(self, motion, a, b);
    if (add != 0) {
        self->field_0x058 += add;
        self->field_0x0A8 = self->field_0x058;
    }
    if (attack != 0) {
        pl_weapon2_attack_set(self, attack, 0);
    }
    if (row->gate_0x08 > 0) {
        pl_act_set_motion_gate(self, row->gate_0x08);
    }
}

/* Advances the actor's charge gauge by one frame, saturating at 999. */
void pl_weapon2_charge_gauge_step(_PLW* self)
{
    if (self->charge_gauge_0x310 < 999) {
        self->charge_gauge_0x310++;
    } else {
        self->charge_gauge_0x310 = 999;
    }
}

/* The charge rate in frames per level the actor's gun skills buy: 30 frames, or the 1.25x / 0.83x
 * skill-adjusted rate. */
s32 pl_charge_cycle_get(_PLW* self)
{
    f32 rate = 30.0f;

    if (Pl_Skill_ck(self, 191) == 1) {
        rate /= 1.25f;
    } else if (Pl_Skill_ck(self, 192) == 1) {
        rate /= 0.83f;
    }
    return (s32)rate;
}

/* The actor's charge level: the charge gauge divided by the skill-adjusted rate, in the byte the
 * charge callers write at +0x312 and clamp. */
u8 pl_charge_stage2_get(_PLW* self)
{
    return (u8)(self->charge_gauge_0x310 / (s16)pl_charge_cycle_get(self));
}

/* Releases a held charge (master actor only): with the release flag up or the hold timer run out, enters the act
 * the charge stage picks from one of the two stage lists; otherwise the follow-up flag enters the recoil act. */
void pl_weapon2_charge_release(_PLW* self, s32 base, s32 mode)
{
    u8 stage;

    if (Pl_master_ck(self) != 0) {
        if ((u32)pl_part_flag2_ck(self, 5) == 1 || self->field_0x378 <= 0) {
            stage = pl_charge_stage2_get(self);
            self->charge_level_0x312 = stage;
            if (stage >= 3) {
                stage = 2;
            }
            if (mode == 0) {
                pl_act_enter(self, 4, pl_weapon2_charge_acts_a[base + stage * 2], 0x10C);
            } else {
                pl_act_enter(self, 4, pl_weapon2_charge_acts_b[base + stage * 2], 0x18C);
            }
        } else if ((u32)pl_part_flag2_ck(self, 7) == 1) {
            if (self->kind_0x09 == 3) {
                pl_act_enter(self, 0, 0x1B, 0x8C);
            } else {
                pl_act_enter(self, 0, 8, 0xC);
            }
        }
    }
}

/* Latches the first landed hit of a weapon-2 swing (+0x565) and shakes the camera (harder for the heavy acts); a
 * bounced hit knocks the actor back.  With no hit by frame `frame`, the act's miss is latched once (+0x566). */
void pl_weapon2_hit_react(_PLW* self, f32 frame)
{
    s32 quake;

    if (self->field_0x565 == 0) {
        do {
            switch (self->act_no) {
            case 4:
            case 16:
            case 18:
            case 26:
            case 35:
            case 36:
                continue;
            case 0:
            case 22:
            case 44:
            case 45:
            case 46:
            case 47:
            case 48:
            case 49:
                quake = 64;
                break;
            default:
                quake = 0;
                break;
            }
            switch (pl_hit_result_best_get(self)) {
            case 2:
                self->field_0x565 = 1;
                camera_pl_quake_req(self, quake);
                break;
            case 3:
            case 4:
                self->field_0x565 = 1;
                camera_pl_quake_req(self, quake | 1);
                break;
            case 0:
            case 1:
            case 5:
                self->field_0x565 = 1;
                camera_pl_quake_req(self, quake);
                if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 4, 27, 0x4080);
                } else {
                    pl_act_enter(self, 4, 5, 0x4000);
                }
                break;
            }
        } while (0);
    }
    if (self->field_0x566 == 0 && self->field_0x565 == 0 && frame > 0.0f &&
        Pl_frame_check(self, 0, frame, 0.0f) == 1) {
        self->field_0x566 = 1;
        pl_act_hold_notify(self, 2);
        camera_pl_quake_req(self, 0);
    }
}

/* The row act on `pl_weapon2_rows_a`: arm the motion, wait for the motion gate and apply the row's second half,
 * then back to the idle slot when the motion ends. */
void pl_weapon2_act_row_a(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_weapon2_row_apply(self, 0, &pl_weapon2_rows_a[kind], (u32)pl_weapon2_steps_row_a);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon2_row_apply(self, 1, &pl_weapon2_rows_a[kind], (u32)pl_weapon2_steps_row_a);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The row act on `pl_weapon2_rows_b`: the step table is picked by the kind, and the first step also clears the
 * actor's frame counter. */
void pl_weapon2_act_row_b(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x28 = 0;
        if (kind == 1) {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_rows_b[kind], (u32)pl_weapon2_steps_row_b1);
        } else {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_rows_b[kind], (u32)pl_weapon2_steps_row_b);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 1) {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_rows_b[kind], (u32)pl_weapon2_steps_row_b1);
            } else {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_rows_b[kind], (u32)pl_weapon2_steps_row_b);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            pl_weapon2_hit_react(self, 56.0f);
        }
        break;
    }
}

/* The lead-in swing: motion 0x458 with attack 3, then motion 0x44E once it ends; back to the idle slot when the
 * second motion ends. */
void pl_weapon2_act_lead_swing(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x458, 2, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_weapon2_attack_set(self, 3, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lead, 0);
        break;
    case 1:
        if ((s32)Get_motion_no(self) == 0x458) {
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x44E, 4, 0x4A);
                Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lead_swing, 0);
            } else {
                pl_weapon2_hit_react(self, 52.0f);
            }
        } else if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The row act on `pl_weapon2_rows_c`: kind 0 drives the first step table, the others the second. */
void pl_weapon2_act_row_c(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        if (kind == 0) {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_rows_c[kind], (u32)pl_weapon2_steps_row_c);
        } else {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_rows_c[kind], (u32)pl_weapon2_steps_row_c1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 0) {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_rows_c[kind], (u32)pl_weapon2_steps_row_c);
            } else {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_rows_c[kind], (u32)pl_weapon2_steps_row_c1);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            pl_weapon2_hit_react(self, 40.0f);
        }
        break;
    }
}

/* A single motion (0x4C5) with its step table; back to the slot-10 motion when it ends. */
void pl_weapon2_act_step_motion(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_step_motion, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 10, 0);
        }
        break;
    }
}

/* The back swing: a wind-up motion (0x46A), then motion 0x46E with the reversed attack 5. */
void pl_weapon2_act_back_swing(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x46A, 0, 0);
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x46E, 0, 8);
            pl_weapon2_attack_set(self, -5, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_back_swing, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The charge act: kinds 0 and 2-6 play their wind-up motion and clear the charge gauge, kind 1 goes straight to the
 * hold; the hold steps the gauge each frame and releases it through `pl_weapon2_charge_release`. */
void pl_weapon2_act_charge(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05 = 1;
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x28 = 0;
        pl_act_arm_flags(self, 1);
        switch (kind) {
        case 0:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            self->act_step_0x05 = 2;
            switch (kind) {
            case 6:
                pl_atk_act_flag_set(self, 1);
                Pl_chr_set_attr_default(self, 0x46C, 0, 0);
                break;
            case 0:
                Pl_chr_set_attr_default(self, 0x46C, 4, 0);
                break;
            case 2:
                Pl_chr_set_attr_default(self, 0x456, 4, 0);
                break;
            case 3:
                Pl_chr_set_attr_default(self, 0x46C, 6, 12);
                break;
            case 4:
                Pl_chr_set_attr_default(self, 0x474, 4, 0);
                break;
            case 5:
                Pl_chr_set_attr_default(self, 0x478, 4, 0);
                break;
            }
            self->field_0x310 = 0;
            eft029_set_half(self, 14);
            pl_act_set_motion_gate(self, 60);
            break;
        case 1:
            self->act_step_0x05 = 3;
            Pl_chr_set_attr_default(self, 0x46D, 8, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_charge, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x46C, 0, 14);
            self->field_0x310 = 0;
            eft029_set_half(self, 14);
            pl_act_set_motion_gate(self, 60);
            pl_act_arm_flags(self, 1);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x46D, 0, 0);
            pl_act_set_motion_gate(self, 1);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_charge, 0);
        }
        break;
    case 3:
        pl_weapon2_charge_gauge_step(self);
        if (Pl_master_ck(self) == 1) {
            pl_act_gauge_gate(self, -1);
        }
        pl_weapon2_charge_release(self, 0, 0);
        break;
    }
}

/* The lunge: the kind's swing and attack; kinds 0, 2 and 3 push the actor forward along its heading by a cosine
 * ramp of the motion frame from frame 30 on. */
void pl_weapon2_act_lunge(_PLW* self, s32 kind)
{
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x46F, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lunge_long, 0);
            pl_weapon2_attack_set(self, 14, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x451, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lunge, 0);
            pl_weapon2_attack_set(self, 6, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x451, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lunge, 0);
            pl_weapon2_attack_set(self, 6, 0);
            break;
        case 3:
            Pl_chr_set_attr_default(self, 0x46F, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lunge_long, 0);
            pl_weapon2_attack_set(self, 14, 0);
            break;
        case 4:
            Pl_chr_set_attr_default(self, 0x451, 2, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_steps_lunge_timed, 0);
            pl_weapon2_attack_set(self, 6, 0);
            pl_act_timer_step(self);
            break;
        }
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (((kind >= 2 && kind <= 3) || kind == 0) && Pl_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            step.x = 0.0f;
            step.y = 0.0f;
            if (kind == 3) {
                step.z = 27.0f * cosf(6.2831855f * (90.0f * (pl_rig_get_motion_frame(self) / 24.0f)) / 360.0f);
            } else {
                step.z = 18.0f * cosf(6.2831855f * (90.0f * (pl_rig_get_motion_frame(self) / 24.0f)) / 360.0f);
            }
            rotVecY(&step, self->field_0x058);
            self->motion_pos_0x3C += step.x;
            self->motion_pos_0x44 += step.z;
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The shell shot: motion 0x452 with attack 8; at frame 74 the shell job fires its kind-0 shell. */
void pl_weapon2_act_shell_fire(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x452, 2, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_weapon2_attack_set(self, 8, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_shell_fire, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 0, 74.0f, 0.0f) == 1) {
            shell_set_func_ptr->method_0x4C(self, 0, shell_set_func_ptr);
        }
        pl_weapon2_hit_react(self, 80.0f);
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The charge hold: motion 0x46B, then the gauge steps each frame; a master actor whose input tier is up turns to
 * the camera and releases the charge, otherwise it enters act 8. */
void pl_weapon2_act_charge_hold(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x46B, 8, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_flag(self, 0xC0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_charge, 0);
        pl_act_arm_flags(self, 1);
    case 1:
        pl_weapon2_charge_gauge_step(self);
        if (Pl_master_ck(self) == 1) {
            pl_act_gauge_gate(self, -1);
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                pl_weapon2_charge_release(self, 1, 0);
            } else {
                pl_act_enter(self, 4, 8, 0x104);
            }
        }
        break;
    }
}

/* The rapid strikes: 2 + the charge level hits (at most 5), each with attacks 11/12, steered by the stick; the
 * last one finishes with motion 0x450 and attack 13. */
void pl_weapon2_act_rapid(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x44F, 6, 0);
        self->field_0x006 = self->charge_level_0x312 + 2;
        if (self->field_0x006 >= 5) {
            self->field_0x006 = 5;
        }
        self->field_0x007 = 0;
        self->field_0x28 = 0;
        self->field_0x0A8 = (u16)self->field_0x058;
        pl_weapon2_attack_set(self, 10, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_rapid, 0);
        break;
    case 1:
        pl_act_set_motion_gate(self, 2);
        if (Pl_master_ck(self) == 1) {
            if (pl_stick_dir_ck(self, 0x800) == 1) {
                self->field_0x0A8 += 0xC0;
            } else if (pl_stick_dir_ck(self, 0x400) == 1) {
                self->field_0x0A8 -= 0xC0;
            }
        }
        if (Pl_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            pl_weapon2_attack_set(self, 11, 0);
        }
        if (Pl_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            pl_weapon2_attack_set(self, 12, 0);
        }
        if (Pl_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            self->field_0x28++;
            if (--self->field_0x006 == 0) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x450, 0, 0);
                self->field_0x0B6 += (u16)3;
                pl_weapon2_attack_set(self, 13, 0);
                Pl_act_set_step_table(self, 0, 0);
            } else if (self->field_0x28 >= 2) {
                Pl_act_set_step_table(self, (u32)pl_weapon2_steps_rapid_loop, 0);
            }
        }
        break;
    case 2:
        pl_act_set_motion_gate(self, 2);
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            pl_weapon2_hit_react(self, 38.0f);
        }
        break;
    }
}

/* A single swing (motion 0x470, attack 15) checked against frame 30; back to the idle slot when it ends. */
void pl_weapon2_act_swing_f(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x470, 4, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_weapon2_attack_set(self, 15, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_swing_f, 0);
        break;
    case 1:
        pl_weapon2_hit_react(self, 30.0f);
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* A single swing (motion 0x46E, attack 16); back to the idle slot when it ends. */
void pl_weapon2_act_swing_g(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x46E, 4, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        pl_weapon2_attack_set(self, 16, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_weapon2_steps_swing_g, 0);
        break;
    case 1:
        pl_weapon2_hit_react(self, -1.0f);
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The alternate-form row act on `pl_weapon2_alt_rows_a` (motion set 3). */
void pl_weapon2_alt_act_row_a(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_a[kind], (u32)pl_weapon2_alt_steps_row_a);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_a[kind], (u32)pl_weapon2_alt_steps_row_a);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form row act on `pl_weapon2_alt_rows_b`: kind 1 drives the second step table. */
void pl_weapon2_alt_act_row_b(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x28 = 0;
        self->act_step_0x05++;
        if (kind == 1) {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_b[kind], (u32)pl_weapon2_alt_steps_row_b1);
        } else {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_b[kind], (u32)pl_weapon2_alt_steps_row_b);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 1) {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_b[kind], (u32)pl_weapon2_alt_steps_row_b1);
            } else {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_b[kind], (u32)pl_weapon2_alt_steps_row_b);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form lead-in swing: a wind-up motion (0x489), then motion 0x480 with attack 19. */
void pl_weapon2_alt_act_lead_swing(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x489, 2, 0);
        pl_act_set_motion_gate(self, 27);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x480, 0, 38);
            pl_weapon2_attack_set(self, 19, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_lead_swing, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form row act on `pl_weapon2_alt_rows_c`: kind 0 drives the first step table. */
void pl_weapon2_alt_act_row_c(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        if (kind == 0) {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_c[kind], (u32)pl_weapon2_alt_steps_row_c);
        } else {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_c[kind], (u32)pl_weapon2_alt_steps_row_c1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 0) {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_c[kind], (u32)pl_weapon2_alt_steps_row_c);
            } else {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_c[kind], (u32)pl_weapon2_alt_steps_row_c1);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form single motion (0x4F7) with its step table; back to the slot-4 motion when it ends. */
void pl_weapon2_alt_act_step_motion(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_step_motion, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* The alternate-form back swing: a wind-up motion (0x49A), then motion 0x488 with the reversed attack 29. */
void pl_weapon2_alt_act_back_swing(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        Pl_chr_set_attr_default(self, 0x49A, 2, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x488, 0, 14);
            pl_weapon2_attack_set(self, -29, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_back_swing, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form charge act: kinds 0 and 2-5 play their wind-up motion, kind 1 goes straight to the hold; the
 * hold steps the gauge and releases it through the alternate stage list. */
void pl_weapon2_alt_act_charge(_PLW* self, s32 kind)
{
    do {
        switch (self->act_step_0x05) {
        case 0:
            self->act_step_0x05 = 1;
            self->field_0x18 = 1;
            pl_act_clear_flag5bb(self);
            Pl_act_set_motion(self, 3, 1, 0);
            self->field_0x28 = 0;
            pl_act_arm_flags(self, 1);
            switch (kind) {
            case 0:
            case 2:
            case 3:
            case 4:
            case 5:
                self->act_step_0x05 = 2;
                switch (kind) {
                case 2:
                    Pl_chr_set_attr_default(self, 0x495, 4, 0);
                    break;
                case 3:
                    Pl_chr_set_attr_default(self, 0x493, 4, 0);
                    break;
                case 4:
                    Pl_chr_set_attr_default(self, 0x499, 4, 0);
                    break;
                case 5:
                    pl_atk_act_flag_set(self, 1);
                default:
                    Pl_chr_set_attr_default(self, 0x482, 4, 0);
                    break;
                }
                self->field_0x310 = 0;
                eft029_set_half(self, 14);
                pl_act_set_motion_gate(self, 60);
                continue;
            case 1:
                self->act_step_0x05 = 3;
                Pl_chr_set_attr_default(self, 0x483, 4, 0);
                Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_charge, 0);
                break;
            default:
                continue;
            }
            break;
        case 1:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x482, 0, 14);
                self->field_0x310 = 0;
                eft029_set_half(self, 14);
                pl_act_set_motion_gate(self, 60);
                pl_act_arm_flags(self, 1);
            }
            continue;
        case 2:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x483, 0, 0);
                pl_act_set_motion_gate(self, 1);
                Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_charge, 0);
            }
            continue;
        case 3:
            break;
        default:
            continue;
        }
        pl_weapon2_charge_gauge_step(self);
        if (Pl_master_ck(self) == 1) {
            pl_act_gauge_gate(self, -1);
        }
        pl_weapon2_charge_release(self, 0, 1);
    } while (0);
}

/* The alternate-form charge hold: motion 0x484, then the gauge steps each frame; a master actor whose input tier is
 * up turns to the camera, adjusts the guard gauge and releases the charge, otherwise it enters act 40. */
void pl_weapon2_alt_act_charge_hold(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x484, 8, 0);
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_flag(self, 0xC0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_charge, 0);
        pl_act_arm_flags(self, 1);
    case 1:
        pl_weapon2_charge_gauge_step(self);
        if (Pl_master_ck(self) == 1) {
            pl_act_gauge_gate(self, -1);
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_camera_heading_update(self);
                pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
                pl_weapon2_charge_release(self, 1, 1);
            } else {
                pl_act_enter(self, 4, 40, 0x184);
            }
        }
        break;
    }
}

/* The alternate-form lunge on `pl_weapon2_alt_rows_lunge`: kind 2 drives the timed step table; kind 1 pushes the
 * actor forward by the cosine ramp from frame 30 on. */
void pl_weapon2_alt_act_lunge(_PLW* self, s32 kind)
{
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        if (kind == 2) {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_lunge[kind], (u32)pl_weapon2_alt_steps_lunge_timed);
            pl_act_timer_step(self);
        } else {
            pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_lunge[kind], (u32)pl_weapon2_alt_steps_lunge);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind != 2) {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_lunge[kind], (u32)pl_weapon2_alt_steps_lunge);
            } else {
                pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_lunge[kind],
                                     (u32)pl_weapon2_alt_steps_lunge_timed);
            }
        }
        break;
    case 2:
        if (kind == 1 && Pl_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            step.x = 0.0f;
            step.y = 0.0f;
            step.z = 18.0f * cosf(6.2831855f * (90.0f * (pl_rig_get_motion_frame(self) / 24.0f)) / 360.0f);
            rotVecY(&step, self->field_0x058);
            self->motion_pos_0x3C += step.x;
            self->motion_pos_0x44 += step.z;
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form lunge with attack 23: kind 1 pushes the actor forward by the cosine ramp from frame 30 on. */
void pl_weapon2_alt_act_lunge_b(_PLW* self, s32 kind)
{
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x486, 2, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_lunge_b, 0);
        pl_act_set_motion_gate(self, 60);
        pl_weapon2_attack_set(self, 23, 0);
        break;
    case 1:
        if (kind == 1 && Pl_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            step.x = 0.0f;
            step.y = 0.0f;
            step.z = 18.0f * cosf(6.2831855f * (90.0f * (pl_rig_get_motion_frame(self) / 24.0f)) / 360.0f);
            rotVecY(&step, self->field_0x058);
            self->motion_pos_0x3C += step.x;
            self->motion_pos_0x44 += step.z;
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form shell shot (attack 24): kind 1 lunges like `pl_weapon2_alt_act_lunge_b`, and at frame 74 the
 * shell job fires its kind-1 shell. */
void pl_weapon2_alt_act_shell_fire(_PLW* self, s32 kind)
{
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x487, 2, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_shell_fire, 0);
        pl_act_set_motion_gate(self, 60);
        pl_weapon2_attack_set(self, 24, 0);
        break;
    case 1:
        if (kind == 1 && Pl_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            step.x = 0.0f;
            step.y = 0.0f;
            step.z = 18.0f * cosf(6.2831855f * (90.0f * (pl_rig_get_motion_frame(self) / 24.0f)) / 360.0f);
            rotVecY(&step, self->field_0x058);
            self->motion_pos_0x3C += step.x;
            self->motion_pos_0x44 += step.z;
        }
        if (Pl_frame_check(self, 0, 74.0f, 0.0f) == 1) {
            shell_set_func_ptr->method_0x4C(self, 1, shell_set_func_ptr);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form rapid strikes (`pl_weapon2_act_rapid` with motions 0x48A/0x48B and attacks 31-34). */
void pl_weapon2_alt_act_rapid(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x48A, 6, 0);
        self->field_0x006 = self->charge_level_0x312 + 2;
        if (self->field_0x006 >= 5) {
            self->field_0x006 = 5;
        }
        self->field_0x007 = 0;
        self->field_0x28 = 0;
        self->field_0x0A8 = (u16)self->field_0x058;
        pl_weapon2_attack_set(self, 34, 0);
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_rapid, 0);
        break;
    case 1:
        pl_act_set_motion_gate(self, 2);
        if (Pl_master_ck(self) == 1) {
            if (pl_stick_dir_ck(self, 0x800) == 1) {
                self->field_0x0A8 += 0xC0;
            } else if (pl_stick_dir_ck(self, 0x400) == 1) {
                self->field_0x0A8 -= 0xC0;
            }
        }
        if (Pl_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            pl_weapon2_attack_set(self, 31, 0);
        }
        if (Pl_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            pl_weapon2_attack_set(self, 32, 0);
        }
        if (Pl_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            self->field_0x28++;
            if (--self->field_0x006 == 0) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x48B, 0, 0);
                self->field_0x0B6 += (u16)3;
                pl_weapon2_attack_set(self, 33, 0);
                Pl_act_set_step_table(self, 0, 0);
            } else if (self->field_0x28 >= 2) {
                Pl_act_set_step_table(self, (u32)pl_weapon2_alt_steps_rapid_loop, 0);
            }
        }
        break;
    case 2:
        pl_act_set_motion_gate(self, 2);
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form row act on `pl_weapon2_alt_rows_d`. */
void pl_weapon2_alt_act_row_d(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_d[kind], (u32)pl_weapon2_alt_steps_row_d);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_d[kind], (u32)pl_weapon2_alt_steps_row_d);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The alternate-form row act on `pl_weapon2_alt_rows_e`. */
void pl_weapon2_alt_act_row_e(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_weapon2_row_apply(self, 0, &pl_weapon2_alt_rows_e[kind], (u32)pl_weapon2_alt_steps_row_e);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon2_row_apply(self, 1, &pl_weapon2_alt_rows_e[kind], (u32)pl_weapon2_alt_steps_row_e);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            pl_weapon2_hit_react(self, -1.0f);
        }
        break;
    }
}

/* The weapon-2 act dispatcher: the actor's act number picks its handler (an unknown act returns to the idle slot);
 * afterwards a charged shell motion plays the charge-level shell sounds. */
void pl_weapon2_act_dispatch(_PLW* self)
{
    nw4r::math::VEC3 pos;
    s32 motion;
    s32 stage;

    VEC3_ctor(&pos);
    switch (self->act_no) {
    case 0:
        pl_weapon2_act_row_a(self, 0);
        break;
    case 1:
        pl_weapon2_act_row_b(self, 0);
        break;
    case 2:
        pl_weapon2_act_row_b(self, 1);
        break;
    case 3:
        pl_weapon2_act_lead_swing(self);
        break;
    case 4:
        pl_weapon2_act_row_c(self, 0);
        break;
    case 5:
        pl_weapon2_act_step_motion(self);
        break;
    case 6:
        pl_weapon2_act_back_swing(self);
        break;
    case 7:
        pl_weapon2_act_charge(self, 0);
        break;
    case 8:
        pl_weapon2_act_charge(self, 1);
        break;
    case 9:
        pl_weapon2_act_charge(self, 2);
        break;
    case 10:
        pl_weapon2_act_charge(self, 3);
        break;
    case 11:
        pl_weapon2_act_lunge(self, 0);
        break;
    case 12:
        pl_weapon2_act_lunge(self, 1);
        break;
    case 13:
        pl_weapon2_act_lunge(self, 3);
        break;
    case 14:
        pl_weapon2_act_lunge(self, 2);
        break;
    case 15:
        pl_weapon2_act_lunge(self, 4);
        break;
    case 16:
        pl_weapon2_act_shell_fire(self);
        break;
    case 17:
        pl_weapon2_act_charge_hold(self);
        break;
    case 18:
        pl_weapon2_act_rapid(self);
        break;
    case 19:
        pl_weapon2_act_row_c(self, 1);
        break;
    case 20:
        pl_weapon2_act_swing_f(self);
        break;
    case 21:
        pl_weapon2_act_swing_g(self);
        break;
    case 22:
        pl_weapon2_alt_act_row_a(self, 0);
        break;
    case 23:
        pl_weapon2_alt_act_row_b(self, 0);
        break;
    case 24:
        pl_weapon2_alt_act_row_b(self, 1);
        break;
    case 25:
        pl_weapon2_alt_act_lead_swing(self);
        break;
    case 26:
        pl_weapon2_alt_act_row_c(self, 0);
        break;
    case 27:
        pl_weapon2_alt_act_step_motion(self);
        break;
    case 28:
        pl_weapon2_alt_act_back_swing(self);
        break;
    case 29:
        pl_weapon2_alt_act_charge(self, 0);
        break;
    case 30:
        pl_weapon2_alt_act_charge_hold(self);
        break;
    case 31:
        pl_weapon2_alt_act_lunge(self, 0);
        break;
    case 32:
        pl_weapon2_alt_act_lunge(self, 1);
        break;
    case 33:
        pl_weapon2_alt_act_lunge_b(self, 0);
        break;
    case 34:
        pl_weapon2_alt_act_lunge_b(self, 1);
        break;
    case 35:
        pl_weapon2_alt_act_shell_fire(self, 0);
        break;
    case 36:
        pl_weapon2_alt_act_rapid(self);
        break;
    case 37:
        pl_weapon2_alt_act_row_d(self, 0);
        break;
    case 38:
        pl_weapon2_act_charge(self, 4);
        break;
    case 39:
        pl_weapon2_act_charge(self, 5);
        break;
    case 40:
        pl_weapon2_alt_act_charge(self, 1);
        break;
    case 41:
        pl_weapon2_alt_act_charge(self, 2);
        break;
    case 42:
        pl_weapon2_alt_act_charge(self, 3);
        break;
    case 43:
        pl_weapon2_alt_act_charge(self, 4);
        break;
    case 44:
        pl_weapon2_act_row_a(self, 1);
        break;
    case 45:
        pl_weapon2_act_row_a(self, 2);
        break;
    case 46:
        pl_weapon2_act_row_a(self, 3);
        break;
    case 47:
        pl_weapon2_alt_act_row_a(self, 1);
        break;
    case 48:
        pl_weapon2_alt_act_row_a(self, 2);
        break;
    case 49:
        pl_weapon2_alt_act_row_a(self, 3);
        break;
    case 50:
        pl_weapon2_alt_act_lunge(self, 2);
        break;
    case 52:
        pl_weapon2_alt_act_row_c(self, 1);
        break;
    case 51:
        pl_weapon2_alt_act_row_e(self, 0);
        break;
    case 53:
        pl_weapon2_act_row_b(self, 2);
        break;
    case 54:
        pl_weapon2_alt_act_row_b(self, 2);
        break;
    case 55:
        pl_weapon2_act_charge(self, 6);
        break;
    case 56:
        pl_weapon2_alt_act_charge(self, 5);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
    motion = Get_motion_no(self);
    if ((motion >= 0x483 && motion <= 0x484) || motion == 0x46B || motion == 0x46D) {
        stage = pl_charge_stage2_get(self);
        if (stage != 0) {
            if (stage == 1) {
                pl_impact_pos_get(self, &pos);
                shell_se_req(self->field_0xAFC, &pos, 13, self->chunk_ofs);
            } else {
                pl_impact_pos_get(self, &pos);
                shell_se_req(self->field_0xAFC, &pos, 14, self->chunk_ofs);
            }
        }
    }
}

/* One gun-ammo record `pl_gun_ammo_entry_get` hands back (fields read by the fire act only). size: 0x4 (approximation:
 * the fire act reads +0x01-+0x03) */
typedef struct GunAmmoEntry {
    /* +0x00 */ u8 ammo_0x00;    /* the ammo kind the record is looked up by */
    /* +0x01 */ u8 shots_0x01;   /* shots per trigger: the rapid-fire count is this minus one */
    /* +0x02 */ u8 power_0x02;   /* the shell's scale in percent */
    /* +0x03 */ u8 reload_0x03;  /* 1-4: picks the chained shot's frame and motion row */
} GunAmmoEntry;

/* The actor's physics block as the fire act reads it: the muzzle position the empty-magazine sound plays at.
 * size: 0xA24 (approximation: only +0xA18 is read) */
typedef struct PlPhysicsMuzzle {
    /* +0x000 */ u8 pad_0x000[0xA18];
    /* +0xA18 */ nw4r::math::VEC3 muzzle_pos_0xA18;
} PlPhysicsMuzzle;

#pragma optimization_level 4

/* 1 when the actor is in act group 4 at one of the acts of `phase` (0: acts 19/21/42/44/46/48, 1: acts 20/22/43/45/47/49);
 * always 0 while the item page owns the screen. */
u32 pl_act4_phase_ck(_PLW* self, s32 phase)
{
    if (pl_item_page2_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A == 4) {
        switch (self->act_no) {
        case 19:
        case 21:
        case 42:
        case 44:
        case 46:
        case 48:
            if (phase == 0) {
                return 1;
            }
            break;
        case 20:
        case 22:
        case 43:
        case 45:
        case 47:
        case 49:
            if (phase == 1) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* Latches +0x276 while a magazine is loaded and the actor's scan rows hold the id-252 guard row. */
void pl_gun_guard_row_latch(_PLW* self)
{
    if (self->field_0x269 != 0 && pl_scan_rows_id252_ck(self) == 1) {
        self->field_0x276 = 1;
    }
}

/* One 22-byte row of the actor's scan table (`_PLW`+0x318, the table `pl_act_step.cpp` walks): the mode
 * byte ends the scan at 0xFF. size: 0x16 */
typedef struct ShellScanRow {
    /* +0x00 */ u8 mode;         /* 0xFF terminates the scan; the low 7 bits are the row's kind */
    /* +0x01 */ u8 pad_0x01[0x1];
    /* +0x02 */ u16 id;           /* 252 marks the row the scan looks for */
    /* +0x04 */ u8 pad_0x04[0x12];
} ShellScanRow;

/* Scans the actor's 22-byte row table for a kind-4 row carrying id 252, stopping at the 0xFF mode
 * byte; the table is only walked while the actor's scan byte is not held off. */
u32 pl_scan_rows_id252_ck(_PLW* self)
{
    ShellScanRow* row = (ShellScanRow*)self->field_0x318;

    if (row == 0 || self->field_0x313 > 0) {
        return 0;
    }
    while (row->mode != 0xFF) {
        if ((row->mode & ~0x80) == 4 && (s32)row->id == 252) {
            return 1;
        }
        row++;
    }
    return 0;
}

/* Builds the actor's weapon-4/5/6 attack entry from an attack value, the caller's kind picking the flag bits. */
void pl_weapon456_attack_set(_PLW* self, s16 value, u8 kind)
{
    if (kind != 0) {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon456_hit_table, (u16)value, pl_weapon456_hit_ids, 4);
    } else {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon456_hit_table, (u16)value, pl_weapon456_hit_ids, 7);
    }
}

/* The gunner's melee bash: motion 0x459 with attack 1 and the weapon class's step table; back to the idle slot when
 * the motion ends. */
void pl_weapon456_act_melee(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_camera_lock_clear(self);
        Pl_act_set_motion(self, 0, 1, 0);
        switch (self->field_0x002) {
        case 4:
            Pl_act_set_step_table(self, (u32)pl_weapon456_steps_melee_4, 0);
            break;
        case 5:
            Pl_act_set_step_table(self, (u32)pl_weapon456_steps_melee_5, 0);
            break;
        case 6:
            Pl_act_set_step_table(self, (u32)pl_weapon456_steps_melee_6, 0);
            break;
        }
        Pl_chr_set_attr_default(self, 0x459, 4, 0);
        pl_weapon456_attack_set(self, 1, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0, 0);
        }
        break;
    }
}

/* The gunner's reload: motion 0x450 with the aim pair 0x45A/0x45B, the magazine refilled up to the loaded count
 * and the motion speed set by the reload tier; back to the aim state when it ends. */
void pl_weapon456_act_reload(_PLW* self)
{
    if (self->camera_lock_0x5E5 != 0) {
        self->camera_lock_0x5E5 = 5;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x450, 0, 0);
        pl_aim_motion_set(self, 0x45A, 0x45B);
        self->field_0x269 = self->ammo_capacity_0x26A;
        if (self->magazine_0x270 < self->ammo_capacity_0x26A) {
            self->field_0x269 = self->magazine_0x270;
        }
        switch (pl_gun_reload_tier_get(self, self->held_item_kind_0x26C)) {
        case 0:
            self->field_0x354 = 1.25f;
            break;
        default:
            self->field_0x354 = 1.0f;
            break;
        case 2:
            self->field_0x354 = 0.75f;
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_state_enter_aim(self, 0, 0, 0, 0x406, 0x407);
        }
        break;
    }
}

/* The gunner's load-and-reload: a load motion picked by the weapon class and form, then the reload of
 * `pl_weapon456_act_reload` at the stored tier; back to the idle slot when it ends. */
void pl_weapon456_act_load_reload(_PLW* self, s32 kind)
{
    if (self->camera_lock_0x5E5 != 0) {
        self->camera_lock_0x5E5 = 5;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        if (kind == 0) {
            if (pl_weapon_alt_form_ck(self) == 0) {
                Pl_chr_set_attr_default(self, 0x46A, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x46B, 4, 0);
            }
        } else if (self->field_0x002 == 6) {
            if (pl_weapon_alt_form_ck(self) == 0) {
                Pl_chr_set_attr_default(self, 0x46A, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x46C, 4, 0);
            }
        } else if (pl_weapon_alt_form_ck(self) == 0) {
            Pl_chr_set_attr_default(self, 0x46A, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x46B, 4, 0);
        }
        self->field_0x006 = 1;
        self->field_0x269 = self->ammo_capacity_0x26A;
        if (self->magazine_0x270 < self->ammo_capacity_0x26A) {
            self->field_0x269 = self->magazine_0x270;
        }
        self->field_0x006 = pl_gun_reload_tier_get(self, self->held_item_kind_0x26C);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x450, 0, 30);
            pl_aim_motion_set(self, 0x45A, 0x45B);
            switch (self->field_0x006) {
            case 0:
                self->field_0x354 = 1.25f;
                break;
            default:
                self->field_0x354 = 1.0f;
                break;
            case 2:
                self->field_0x354 = 0.75f;
                break;
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0, 0);
        }
        break;
    }
}

/* The gunner's ready motion (0x458) with the aim pair 0x45C/0x45D; back to the aim state when it ends. */
void pl_weapon456_act_ready(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x458, 0, 0);
        pl_aim_motion_set(self, 0x45C, 0x45D);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_state_enter_aim(self, 0, 0, 0, 0x406, 0x407);
        }
        break;
    }
}

/* Plays the shot sound of the actor's current fire motion (shot 0/1/2 by motion, both forms) at the gunner
 * position, in the actor's own area only. */
void pl_weapon456_shot_se(_PLW* self, s32 alt)
{
    nw4r::math::VEC3 pos;
    u32 shot = 0xFF;

    VEC3_ctor(&pos);
    if (pl_now_area_ck(self) != 0) {
        switch (Get_motion_no(self)) {
        case 0x451:
        case 0x482:
            shot = 0;
            break;
        case 0x44D:
        case 0x47E:
            shot = 1;
            break;
        case 0x454:
        case 0x485:
            shot = 2;
            break;
        }
        if (shot != 0xFF) {
            Pl_get_gunner_pos(self, &pos, 1);
            pl_gun_shot_se_req(self, &pos, alt, shot);
        }
    }
}

/* The gunner's fire act: the `index` fire motion of the form's table (or the charge motion for `mode` 1), the shot
 * at frame 4 with its sound, shell, ammo use and empty-magazine sound; rapid shots (+0x007) chain the next motion,
 * and the alternate form's indices 2/5 recoil the actor along its aim. */
void pl_weapon456_act_fire(_PLW* self, s32 index, s32 mode, s32 alt, u8 special)
{
    PlGunFireMotion* rows;
    u32 steps;
    s32 next;
    f32 frame;
    f32 scale;
    GunAmmoEntry* entry;

    steps = 0;
    if (self->camera_lock_0x5E5 != 0) {
        self->camera_lock_0x5E5 = 5;
    }
    rows = (alt == 0) ? pl_weapon456_fire_motions : pl_weapon456_alt_fire_motions;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (special == 0) {
            self->field_0x007 = 0;
        } else {
            entry = (GunAmmoEntry*)pl_gun_ammo_entry_get(self, self->held_item_kind_0x26C);
            if (entry == 0) {
                self->field_0x007 = 0;
            } else {
                self->field_0x007 = entry->shots_0x01 - 1;
                if (Pl_Skill_ck(self, 0xCB) == 1) {
                    self->field_0x007++;
                }
            }
        }
        self->field_0x058 = self->field_0x0A8;
        self->param_0x54 = self->field_0x0AC;
        if (alt == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
        }
        if (mode == 1) {
            if (alt == 0) {
                Pl_chr_set_attr_default(self, 0x46E, 2, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x4A0, 2, 0);
            }
        } else {
            Pl_chr_set_attr_default(self, rows[index].motion, 0, 0);
            pl_aim_motion_set(self, rows[index].up_motion, rows[index].down_motion);
            self->act_step_0x05 = 2;
        }
        if (alt == 0) {
            switch (self->field_0x002) {
            case 5:
                steps = pl_weapon456_fire_steps_5[index];
                break;
            case 6:
                steps = pl_weapon456_fire_steps_6[index];
                break;
            }
        } else if (self->field_0x002 != 4) {
            steps = pl_weapon456_alt_fire_steps[index];
        }
        if (steps != 0 && special == 0) {
            Pl_act_set_step_table(self, steps, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (mode == 1) {
                Pl_chr_set_attr_default(self, rows[index].motion, 0, 2);
            } else {
                Pl_chr_set_attr_default(self, rows[index].motion, 0, 0);
            }
            pl_aim_motion_set(self, rows[index].up_motion, rows[index].down_motion);
        }
        break;
    case 2:
    case 3:
        if (Pl_frame_check(self, 0, 4.0f, 0.0f) == 1 && self->act_step_0x05 == 2) {
            self->act_step_0x05++;
            pl_act_hold_notify(self, 0);
            camera_pl_quake2_req(self, 1);
            if (special == 0) {
                pl_weapon456_shot_se(self, alt);
                shell_set_func_ptr->method_0x44(self, self->held_item_kind_0x26C, shell_set_func_ptr, 1.0f);
            } else {
                entry = (GunAmmoEntry*)pl_gun_ammo_entry_get(self, self->held_item_kind_0x26C);
                if (entry == 0) {
                    pl_weapon456_shot_se(self, alt);
                    shell_set_func_ptr->method_0x44(self, self->held_item_kind_0x26C, shell_set_func_ptr, 1.0f);
                } else {
                    scale = entry->power_0x02;
                    scale *= 0.01f;
                    pl_weapon456_shot_se(self, alt);
                    shell_set_func_ptr->method_0x44(self, self->held_item_kind_0x26C, shell_set_func_ptr, scale);
                }
            }
            if (self->field_0x006 == 0) {
                self->field_0x006++;
                self->field_0x269--;
                if (Pl_motion_input_ck(1) == 0 && pl_item_slot_item_get(self, self->item_slot_sel) != 0x35) {
                    pl_item_add(self, pl_item_slot_item_get(self, self->item_slot_sel), -1);
                }
                if (self->magazine_0x270 == 0) {
                    self->field_0x269 = 0;
                }
                self->magazine_shown_0x272 = self->magazine_0x270;
                self->capacity_shown_0x275 = self->ammo_capacity_0x26A;
                self->field_0x274 = self->field_0x269;
            }
            if (self->field_0x007 == 0 && self->field_0x269 == 0 && Pl_master_ck(self) == 1 &&
                pl_now_area_ck(self) == 1) {
                pl_gun_empty_se_req(self, &((PlPhysicsMuzzle*)self->physics_0x13C)->muzzle_pos_0xA18);
            }
        }
        if (self->field_0x5E6 == 0 && Pl_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            eft042_set((MHchar*)self, 2);
        }
        if (Pl_motion_end_ck(self) == 1) {
            if (alt == 0) {
                pl_act_state_enter_aim(self, 0, 0, 0, 0x406, 0x407);
            } else {
                pl_act_state_enter_aim(self, 3, 6, 0, 0x438, 0x439);
            }
            break;
        }
        if (alt == 1 && (index == 2 || index == 5)) {
            pl_model_set_state(self, 1);
            if (Pl_frame_check(self, 3, 8.0f, 110.0f) == 1) {
                nw4r::math::VEC3 step;
                nw4r::math::VEC3 pos;
                _CP_VECTOR aim;

                VEC3_ctor(&step);
                VEC3_ctor(&pos);
                Pl_get_gunner_pos(self, &pos, 0);
                Pl_get_gunner_vec(self, &aim);
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = -10.0f * cosf(6.2831855f * (90.0f * ((pl_rig_get_motion_frame(self) - 8.0f) / 102.0f)) /
                                       360.0f);
                rotVecX(&step, aim.x);
                rotVecY(&step, aim.y);
                self->motion_pos_0x3C += step.x;
                self->motion_pos_0x40 += step.y;
                self->motion_pos_0x44 += step.z;
            }
        }
        if (self->field_0x007 != 0) {
            entry = (GunAmmoEntry*)pl_gun_ammo_entry_get(self, self->held_item_kind_0x26C);
            if (entry == 0) {
                frame = 26.0f;
                next = 2;
            } else {
                switch (entry->reload_0x03) {
                case 1:
                    frame = 26.0f;
                    next = 0;
                    break;
                case 2:
                    frame = 26.0f;
                    next = 1;
                    break;
                case 3:
                    frame = 34.0f;
                    next = 1;
                    break;
                case 4:
                    frame = 26.0f;
                    next = 2;
                    break;
                }
            }
            if (Pl_frame_check(self, 1, frame, 0.0f) == 1) {
                self->act_step_0x05 = 2;
                self->field_0x007--;
                next = (self->field_0x007 != 0) ? 0 : next;
                if (self->field_0x0B6 & 0x8000) {
                    self->field_0x0B6 = (self->field_0x0B6 << 1) | 1;
                } else {
                    self->field_0x0B6 = self->field_0x0B6 << 1;
                }
                Pl_chr_set_attr_default(self, rows[next].motion, 0, 0);
                pl_aim_motion_set(self, rows[next].up_motion, rows[next].down_motion);
            }
        }
        break;
    }
}

/* The alternate-form reload: motion 0x481 with the aim pair 0x48B/0x48C, the magazine refilled and the motion speed
 * set by the reload tier; back to the alternate aim state when it ends. */
void pl_weapon456_alt_act_reload(_PLW* self)
{
    if (self->camera_lock_0x5E5 != 0) {
        self->camera_lock_0x5E5 = 5;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 1;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x481, 0, 0);
        pl_aim_motion_set(self, 0x48B, 0x48C);
        self->field_0x269 = self->ammo_capacity_0x26A;
        if (self->magazine_0x270 < self->ammo_capacity_0x26A) {
            self->field_0x269 = self->magazine_0x270;
        }
        switch (pl_gun_reload_tier_get(self, self->held_item_kind_0x26C)) {
        case 0:
            self->field_0x354 = 1.25f;
            break;
        default:
            self->field_0x354 = 1.0f;
            break;
        case 2:
            self->field_0x354 = 0.75f;
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_state_enter_aim(self, 3, 0, 0, 0x438, 0x439);
        }
        break;
    }
}

/* The alternate-form ready motion (0x489) with the aim pair 0x48D/0x48E. */
void pl_weapon456_alt_act_ready(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x489, 0, 0);
        pl_aim_motion_set(self, 0x48D, 0x48E);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_state_enter_aim(self, 3, 0, 0, 0x438, 0x439);
        }
        break;
    }
}

/* The alternate-form load-and-reload (`pl_weapon456_act_load_reload` with motions 0x49C-0x49E and 0x481). */
void pl_weapon456_alt_act_load_reload(_PLW* self, s32 kind)
{
    if (self->camera_lock_0x5E5 != 0) {
        self->camera_lock_0x5E5 = 5;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        self->field_0x18 = 1;
        if (self->field_0x002 == 6) {
            if (kind == 0) {
                if (pl_weapon_alt_form_ck(self) == 1) {
                    Pl_chr_set_attr_default(self, 0x49D, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x49C, 4, 0);
                }
            } else if (pl_weapon_alt_form_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x49E, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x49C, 4, 0);
            }
        } else if (pl_weapon_alt_form_ck(self) == 1) {
            Pl_chr_set_attr_default(self, 0x49D, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x49C, 4, 0);
        }
        self->field_0x006 = 1;
        self->field_0x269 = self->ammo_capacity_0x26A;
        if (self->magazine_0x270 < self->ammo_capacity_0x26A) {
            self->field_0x269 = self->magazine_0x270;
        }
        self->field_0x006 = pl_gun_reload_tier_get(self, self->held_item_kind_0x26C);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x481, 0, 30);
            pl_aim_motion_set(self, 0x48B, 0x48C);
            switch (self->field_0x006) {
            case 0:
                self->field_0x354 = 1.25f;
                break;
            default:
                self->field_0x354 = 1.0f;
                break;
            case 2:
                self->field_0x354 = 0.75f;
                break;
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 0, 0);
        }
        break;
    }
}

/* The alternate-form melee bash: motion 0x48A with attack 1 and the weapon class's step table. */
void pl_weapon456_alt_act_melee(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_camera_lock_clear(self);
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x48A, 4, 0);
        switch (self->field_0x002) {
        case 4:
            Pl_act_set_step_table(self, (u32)pl_weapon456_alt_steps_melee_4, 0);
            break;
        case 5:
        case 6:
            Pl_act_set_step_table(self, (u32)pl_weapon456_alt_steps_melee_56, 0);
            break;
        }
        pl_weapon456_attack_set(self, 1, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 0, 0);
        }
        break;
    }
}

/* The crouch fire: motion 0x3E9 (form 0) or 0x41A (alternate form) with effect 43 on for five frames, held for the
 * ammo's frame count while the stick steers; then the `index` follow-up act of the form's list. */
void pl_weapon456_act_crouch_fire(_PLW* self, s32 index, s32 alt)
{
    s32 ammo;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x36A = 6;
        if (alt == 0) {
            Pl_act_set_motion(self, 0, 1, 0);
            Pl_chr_set_attr_default(self, 0x3E9, 4, 0);
            pl_aim_motion_set(self, 0x406, 0x407);
        } else {
            Pl_act_set_motion(self, 3, 1, 0);
            Pl_chr_set_attr_default(self, 0x41A, 4, 0);
            pl_aim_motion_set(self, 0x438, 0x439);
        }
        eft043_set(self, 1);
        switch (self->held_item_kind_0x26C) {
        default:
            ammo = 0;
            break;
        case 40:
            ammo = 1;
            break;
        case 41:
            ammo = 2;
            break;
        }
        self->field_0x28 = pl_weapon456_crouch_frames[ammo];
        self->field_0x006 = 5;
        self->field_0x058 = self->field_0x0A8;
        self->param_0x54 = self->field_0x0AC;
        break;
    case 1:
        if (self->field_0x006 != 0) {
            if (--self->field_0x006 == 0) {
                eft043_set(self, 0);
            }
        }
        if (Pl_master_ck(self) != 0) {
            if (--self->field_0x28 <= 0) {
                if (alt == 0) {
                    pl_act_enter_raw(self, 4, pl_weapon456_crouch_acts[index], 5);
                } else {
                    pl_act_enter_raw(self, 4, pl_weapon456_alt_crouch_acts[index], 0x85);
                }
            } else if (alt == 0) {
                pl_motion_stick_move(self, 0x3E9, 0);
            } else {
                pl_motion_stick_move(self, 0x41A, 0);
            }
        }
        break;
    }
}

/* The weapon-4/5/6 act dispatcher: the actor's act number picks its handler and its arguments. */
void pl_weapon456_act_dispatch(_PLW* self)
{
    switch (self->act_no) {
    case 0:
        pl_weapon456_act_melee(self);
        break;
    case 1:
        pl_weapon456_act_reload(self);
        break;
    case 2:
        pl_weapon456_act_load_reload(self, 0);
        break;
    case 3:
        pl_weapon456_act_load_reload(self, 1);
        break;
    case 4:
        pl_weapon456_act_ready(self);
        break;
    case 5:
        pl_weapon456_act_fire(self, 0, 0, 0, 0);
        break;
    case 6:
        pl_weapon456_act_fire(self, 1, 0, 0, 0);
        break;
    case 7:
        pl_weapon456_act_fire(self, 2, 0, 0, 0);
        break;
    case 8:
        pl_weapon456_act_fire(self, 0, 1, 0, 0);
        break;
    case 9:
        pl_weapon456_act_fire(self, 1, 1, 0, 0);
        break;
    case 10:
        pl_weapon456_act_fire(self, 2, 1, 0, 0);
        break;
    case 11:
        pl_weapon456_act_fire(self, 0, 0, 1, 0);
        break;
    case 12:
        pl_weapon456_act_fire(self, 1, 0, 1, 0);
        break;
    case 13:
        pl_weapon456_act_fire(self, 2, 0, 1, 0);
        break;
    case 14:
        pl_weapon456_alt_act_reload(self);
        break;
    case 15:
        pl_weapon456_alt_act_ready(self);
        break;
    case 16:
        pl_weapon456_alt_act_load_reload(self, 0);
        break;
    case 17:
        pl_weapon456_alt_act_load_reload(self, 1);
        break;
    case 18:
        pl_weapon456_alt_act_melee(self);
        break;
    case 19:
        pl_weapon456_act_crouch_fire(self, 0, 0);
        break;
    case 20:
        pl_weapon456_act_fire(self, 3, 0, 0, 0);
        break;
    case 21:
        pl_weapon456_act_crouch_fire(self, 0, 1);
        break;
    case 22:
        pl_weapon456_act_fire(self, 3, 0, 1, 0);
        break;
    case 23:
        pl_weapon456_act_fire(self, 0, 1, 1, 0);
        break;
    case 24:
        pl_weapon456_act_fire(self, 1, 1, 1, 0);
        break;
    case 25:
        pl_weapon456_act_fire(self, 2, 1, 1, 0);
        break;
    case 26:
        pl_weapon456_act_fire(self, 0, 0, 0, 1);
        break;
    case 27:
        pl_weapon456_act_fire(self, 0, 0, 0, 2);
        break;
    case 28:
        pl_weapon456_act_fire(self, 0, 0, 0, 3);
        break;
    case 29:
        pl_weapon456_act_fire(self, 0, 0, 0, 4);
        break;
    case 30:
        pl_weapon456_act_fire(self, 0, 1, 0, 1);
        break;
    case 31:
        pl_weapon456_act_fire(self, 0, 1, 0, 2);
        break;
    case 32:
        pl_weapon456_act_fire(self, 0, 1, 0, 3);
        break;
    case 33:
        pl_weapon456_act_fire(self, 0, 1, 0, 4);
        break;
    case 34:
        pl_weapon456_act_fire(self, 0, 0, 1, 1);
        break;
    case 35:
        pl_weapon456_act_fire(self, 0, 0, 1, 2);
        break;
    case 36:
        pl_weapon456_act_fire(self, 0, 0, 1, 3);
        break;
    case 37:
        pl_weapon456_act_fire(self, 0, 0, 1, 4);
        break;
    case 38:
        pl_weapon456_act_fire(self, 0, 1, 1, 1);
        break;
    case 39:
        pl_weapon456_act_fire(self, 0, 1, 1, 2);
        break;
    case 40:
        pl_weapon456_act_fire(self, 0, 1, 1, 3);
        break;
    case 41:
        pl_weapon456_act_fire(self, 0, 1, 1, 4);
        break;
    case 42:
        pl_weapon456_act_crouch_fire(self, 1, 0);
        break;
    case 43:
        pl_weapon456_act_fire(self, 4, 0, 0, 0);
        break;
    case 44:
        pl_weapon456_act_crouch_fire(self, 1, 1);
        break;
    case 45:
        pl_weapon456_act_fire(self, 4, 0, 1, 0);
        break;
    case 46:
        pl_weapon456_act_crouch_fire(self, 2, 0);
        break;
    case 48:
        pl_weapon456_act_crouch_fire(self, 2, 1);
        break;
    case 47:
        pl_weapon456_act_fire(self, 5, 0, 0, 0);
        break;
    case 49:
        pl_weapon456_act_fire(self, 5, 0, 1, 0);
        break;
    }
}

#pragma optimization_level reset

/* Builds the actor's own attack entry from a signed attack value and the row's flag pair, the kind
 * selecting which pair (`fn_80284204`'s shape with the row's own flag set). */
void pl_act_attack_set(_PLW* self, s16 value, u8 kind)
{
    u16 index;
    u16 flags;

    if (value < 0) {
        index = (u16)-value;
        flags = 16;
    } else {
        index = (u16)value;
        flags = 0;
    }
    switch (kind) {
    case 0:
        pl_hit_attack_build(self, &self->hit_0x484, pl_class7_hit_table, index, pl_class7_hit_ids, (u16)(flags | 7));
        break;
    case 1:
        pl_hit_attack_build(self, &self->hit_0x484, pl_class7_hit_table, index, pl_class7_hit_ids, (u16)(flags | 4));
        break;
    case 2:
        pl_hit_attack_build(self, &self->hit_0x484, pl_class7_hit_table, index, pl_class7_hit_ids, (u16)(flags | 263));
        break;
    }
}


/* Applies one motion row: its plain motion, or (`mode` 1, or a row without one) its step motion with `step_table`
 * and its attack entry; `mode` 0 on the step half also advances the act step byte. */
void pl_act_motion_row_apply(_PLW* self, u8 mode, const PlMotionRow* row, u32 step_table)
{
    u32 motion;
    s32 a;
    s32 b;
    s32 attack;

    if (mode == 1 || row->motion == 0xFFFF) {
        motion = row->step_motion;
        a = row->step_motion_a;
        b = row->step_motion_b;
        attack = row->attack;
        Pl_act_set_step_table(self, step_table, 0);
        if (mode == 0) {
            self->act_step_0x05++;
        }
    } else {
        motion = row->motion;
        a = row->motion_a;
        b = row->motion_b;
        attack = 0;
    }
    Pl_chr_set_attr_default(self, motion, a, b);
    if (attack != 0) {
        pl_act_attack_set(self, attack, 0);
    }
}

} /* extern "C" */
