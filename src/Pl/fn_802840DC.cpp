/*
 * Pl/fn_802840DC.cpp - the player's shell (gunner shot) band: the per-kind act-step machines, the attack-row
 *   appliers and the actor's own attack entry.
 * RANGE. .text 0x802840DC-0x80288CEC (54 functions); .data 0x805C9568-0x805CBFC0, .sdata 0x80792188-0x807921B0,
 *   .sdata2 0x8079A1D8-0x8079A270, extab, extabindex.
 * NAMES. The file and functions keep the map's stems (no `__FILE__` string covers the band; the runtime dump answers
 *   placeholders).
 * RESIDUALS. 41 functions unwritten (objdiff scores them 0) in 5 runs: 0x802844B0-0x80284780, 0x802849F0-0x80286BF0,
 *   0x80286DF8-0x8028732C, 0x802873D4-0x80288B98, 0x80288C2C-0x80288CEC.  `fn_802872E4` waits for its two `_PLW`
 *   fields (`unk269`/`unk276`), whose meaning the band does not establish.
 *  - `fn_8028732C`: its two early `return 0`s are laid out as one block at the end where the target keeps the
 *    `p == 0` return before the loop, and the target reads the 22-byte row's id with `lhz` + `cmpwi` where the
 *    `s16 id` gives `lha` (an unsigned load with a signed compare is not reachable from either spelling).
 *  - flipcheck: the object emits no `.sdata` (0x28 claimed) or `.sdata2` (0x98); `.text` 0x990, `.data` 0x8C, extab
 *    0x40 and extabindex 0x60 against the claims 0x4C10, 0x2A58, 0x180 and 0x240; every compared section differs.
 * SHAPES. The `.sdata` id lists (`lbl_80792188`, `lbl_80792190`, `lbl_807921A8`) are sized extern arrays: the target
 *   addresses them through r13 (`li r7, lbl_80792188@sda21`), which MWCC emits only for a sized declaration.
 */

#include "types.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_802840DC.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "unsplit/Pl.h"

extern "C" {

/* The actor's current shell act (`_PLW::act_no`, 0-34) dispatched to its per-act handler; an act
 * number outside the table does nothing. */
void fn_802840DC(_PLW* self)
{
    switch (self->act_no) {
    case 0:
        fn_80282CCC(self, 0);
        return;
    case 1:
        fn_80282CCC(self, 1);
        return;
    case 2:
        fn_80282CCC(self, 2);
        return;
    case 3:
        fn_80282CCC(self, 3);
        return;
    case 4:
        fn_80282ED8(self, 0);
        return;
    case 5:
        fn_80282ED8(self, 1);
        return;
    case 6:
        fn_80283010(self, 0);
        return;
    case 7:
        fn_80283010(self, 1);
        return;
    case 8:
        fn_80283010(self, 2);
        return;
    case 9:
        fn_802831EC(self, 0);
        return;
    case 10:
        fn_802831EC(self, 1);
        return;
    case 11:
        fn_802831EC(self, 2);
        return;
    case 12:
        fn_802833F4(self, 0);
        return;
    case 13:
        fn_802833F4(self, 1);
        return;
    case 14:
        fn_8028350C(self);
        return;
    case 15:
        fn_802835D8(self);
        return;
    case 16:
        fn_802836A4(self, 0);
        return;
    case 17:
        fn_802836A4(self, 1);
        return;
    case 18:
        fn_802836A4(self, 2);
        return;
    case 19:
        fn_80283810(self, 1);
        return;
    case 20:
        fn_80283810(self, 0);
        return;
    case 21:
        fn_802839A8(self, 0);
        return;
    case 22:
        fn_802839A8(self, 1);
        return;
    case 23:
        fn_80283B88(self);
        return;
    case 24:
        fn_80283C60(self, 0);
        return;
    case 25:
        fn_80283DB0(self);
        return;
    case 26:
        fn_80283E80(self);
        return;
    case 27:
        fn_80283F24(self);
        return;
    case 28:
        fn_802831EC(self, 3);
        return;
    case 29:
        fn_802831EC(self, 4);
        return;
    case 30:
        fn_80283FC8(self, 0);
        return;
    case 31:
        fn_80283FC8(self, 1);
        return;
    case 32:
        fn_80283FC8(self, 2);
        return;
    case 33:
        fn_802839A8(self, 2);
        return;
    case 34:
        fn_80283C60(self, 1);
        return;
    }
}

/* Builds the actor's own attack entry from a signed attack value: the magnitude is the attack index, the sign,
 * the attack-flag predicate and the caller's flag pick the entry's flag bits; then runs the shared set-up. */
void fn_80284204(_PLW* self, s16 value, u8 flag)
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
        fn_80277974(self, &self->hit_0x484, lbl_805C9608, magnitude, lbl_80792188,
                    (u16)(flags | 4));
    } else {
        fn_80277974(self, &self->hit_0x484, lbl_805C9608, magnitude, lbl_80792188,
                    (u16)(flags | 7));
    }
}

/* Applies one attack row: picks the field set the kind/mode selects, sets the motion, adds the row's frame
 * offset onto the actor's frame counters and runs the row's attack value and motion gate. */
void fn_802842D4(_PLW* self, u8 kind, ShellAtkRow* row, u32 table)
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
        fn_80284204(self, attack, 0);
    }
    if (row->gate_0x08 > 0) {
        fn_80277C50(self, row->gate_0x08);
    }
}

/* Advances the actor's charge gauge by one frame, saturating at 999. */
void fn_802843CC(_PLW* self)
{
    if (self->charge_gauge_0x310 < 999) {
        self->charge_gauge_0x310++;
    } else {
        self->charge_gauge_0x310 = 999;
    }
}

/* The charge rate in frames per level the actor's gun skills buy: 30 frames, or the 1.25x / 0.83x
 * skill-adjusted rate. */
s32 fn_802843F0(_PLW* self)
{
    f32 rate = lbl_8079A1D8;

    if (Pl_Skill_ck(self, 191) == 1) {
        rate /= lbl_8079A1DC;
    } else if (Pl_Skill_ck(self, 192) == 1) {
        rate /= lbl_8079A1E0;
    }
    return (s32)rate;
}

/* The actor's charge level: the charge gauge divided by the skill-adjusted rate, in the byte the
 * charge callers write at +0x312 and clamp. */
u8 fn_80284474(_PLW* self)
{
    return (u8)(self->charge_gauge_0x310 / (s16)fn_802843F0(self));
}

/* The band's motion gate and scan helper used by the act machines below (`fn_802845F0` and
 * `fn_8028732C` are later functions of this unit). */
void fn_802845F0(_PLW* self, f32 value);
u32 fn_8028732C(_PLW* self);

/* The band's first act-step machine: arm the motion, wait for the motion gate and apply the next `lbl_805C9A78`
 * row, then run the follow-up motion or hand the motion gate the band's -1 value. */
void fn_80284780(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        fn_802842D4(self, 0, &lbl_805C9A78[kind], (u32)lbl_805C9DC8);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            fn_802842D4(self, 1, &lbl_805C9A78[kind], (u32)lbl_805C9DC8);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            fn_802845F0(self, lbl_8079A1E8);
        }
        break;
    }
}

/* The sibling act-step machine for the shell kind whose rows are `lbl_805C9AC0`: the row's companion
 * table is picked by the kind, and the first step also clears the actor's frame counter. */
void fn_80284884(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x28 = 0;
        if (kind == 1) {
            fn_802842D4(self, 0, &lbl_805C9AC0[kind], (u32)lbl_805C9EBC);
        } else {
            fn_802842D4(self, 0, &lbl_805C9AC0[kind], (u32)lbl_805C9E4C);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (kind == 1) {
                fn_802842D4(self, 1, &lbl_805C9AC0[kind], (u32)lbl_805C9EBC);
            } else {
                fn_802842D4(self, 1, &lbl_805C9AC0[kind], (u32)lbl_805C9E4C);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else {
            fn_802845F0(self, lbl_8079A1EC);
        }
        break;
    }
}

/* One 22-byte row of the actor's scan table (`_PLW`+0x318, the table `fn_8025E298` walks): the mode
 * byte ends the scan at 0xFF. size: 0x16 */
typedef struct ShellScanRow {
    /* +0x00 */ u8 mode;         /* 0xFF terminates the scan; the low 7 bits are the row's kind */
    /* +0x01 */ u8 pad_0x01[0x1];
    /* +0x02 */ s16 id;           /* 252 marks the row the scan looks for */
    /* +0x04 */ u8 pad_0x04[0x12];
} ShellScanRow;

/* Scans the actor's 22-byte row table for a kind-4 row carrying id 252, stopping at the 0xFF mode
 * byte; the table is only walked while the actor's scan byte is not held off. */
u32 fn_8028732C(_PLW* self)
{
    ShellScanRow* row = (ShellScanRow*)self->field_0x318;

    if (row == 0) {
        return 0;
    }
    if (self->field_0x313 > 0) {
        return 0;
    }
    while (row->mode != 0xFF) {
        if ((row->mode & ~0x80) == 4 && row->id == 252) {
            return 1;
        }
        row++;
    }
    return 0;
}

/* Builds the actor's own attack entry from a signed attack value the way `fn_80284204` does, with the
 * row's flag pair chosen by the caller's kind. */
void fn_8028738C(_PLW* self, s16 value, u8 kind)
{
    if (kind != 0) {
        fn_80277974(self, &self->hit_0x484, lbl_805CB0B0, (u16)value, lbl_80792190, 4);
    } else {
        fn_80277974(self, &self->hit_0x484, lbl_805CB0B0, (u16)value, lbl_80792190, 7);
    }
}

/* The act-step machine for the shell kind whose rows are `lbl_805C9CAC`: arm the motion, wait for the motion
 * gate and apply the next row, then run the follow-up motion or fall back to the band's motion gate. */
void fn_80286BF0(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        fn_802842D4(self, 0, &lbl_805C9CAC[kind], (u32)lbl_805CA938);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            fn_802842D4(self, 1, &lbl_805C9CAC[kind], (u32)lbl_805CA938);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            fn_802845F0(self, lbl_8079A1E8);
        }
        break;
    }
}

/* The sibling act-step machine whose rows are `lbl_805C9CD0` and whose companion table is
 * `lbl_805CACC4`. */
void fn_80286CF4(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        fn_802842D4(self, 0, &lbl_805C9CD0[kind], (u32)lbl_805CACC4);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            fn_802842D4(self, 1, &lbl_805C9CD0[kind], (u32)lbl_805CACC4);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else {
            fn_802845F0(self, lbl_8079A1E8);
        }
        break;
    }
}

/* Builds the actor's own attack entry from a signed attack value and the row's flag pair, the kind
 * selecting which pair (`fn_80284204`'s shape with the row's own flag set). */
void fn_80288B98(_PLW* self, s16 value, u8 kind)
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
        fn_80277974(self, &self->hit_0x484, lbl_805CBC80, index, lbl_807921A8, (u16)(flags | 7));
        break;
    case 1:
        fn_80277974(self, &self->hit_0x484, lbl_805CBC80, index, lbl_807921A8, (u16)(flags | 4));
        break;
    case 2:
        fn_80277974(self, &self->hit_0x484, lbl_805CBC80, index, lbl_807921A8, (u16)(flags | 263));
        break;
    }
}

} /* extern "C" */
