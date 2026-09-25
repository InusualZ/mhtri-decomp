/* auto/8014A1BC_fn_8014A1BC.c - the enemy-work state unit, 0x8014A1BC..0x801502C8 (60 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * Provisional attribution: the range came from the DOL's own layout (docs/plan.md 12 item 5).  Seams
 * (see the `tu-boundary-discovery` skill): .data run jump jumptable_805A30B0 -> jumptable_805A4020,
 * .sdata2 run jump lbl_80796FD4 -> lbl_80796FD8.
 *
 * What it is: the state machines of one enemy action - two jump-table dispatchers (`fn_8014AA4C`,
 * `fn_8014B5C0`, `fn_8014E3AC`) tail-call a handler per action code, and each handler walks the
 * object's two-level `state`/`sub_state` machine, driving the frame checks, motions, scales and the
 * shell effect callbacks.
 *
 * Result: 59 of 60 functions at or above 80 % (14 at 100 %, mean 96.18 %, size-weighted 95.42 %).
 * `Object(NonMatching, ...)`: the object does not link.  The residual is in fn_8014BDAC (70.26 %):
 * retail keeps `clrlwi r0,r5,24; cmpwi r0,0` separate where every source form tried fuses the mask
 * and the zero-compare into `clrlwi.`; see `.pi/notes/8014a1bc-fn-8014a1bc-03ce.md`.  fn_8014CEF8
 * (81.36 %) and fn_8014F078 (85.42 %) are closed but low because their stack locals were typed VEC3
 * from their use sites.
 *
 * Object: `_ENEMY_WORK` (name evidence: the mangled callee `em_frame_check__FP11_ENEMY_WORKUsff`
 * carries the 11-character type name `_ENEMY_WORK`).  Field offsets and widths are read from the
 * target's load/store instructions, and the `VEC3`s at +0x188 / +0x1B0 / +0x310 / +0x36C from the
 * `PQ34nw4r4math4VEC3` arguments they are passed as.  Size annotations are lower bounds.
 *
 * Load-bearing source shapes (each measured; see the notes file for the full list):
 *   * a `switch` whose `default:` is written first where retail puts the default body after the
 *     compare chain (`fn_8014B6CC`, `fn_8014C16C`);
 *   * `(x & 0xFF)`, not `(u8)x`, when retail uses the signed `cmpwi`;
 *   * `>=`, not `==`, for the `fcmpo` + `cror eq,gt,eq` float tests.
 *
 * Language: C (the unit's own symbols are plain, no `__FILE__` string, no mangled definition).
 *
 * Data runs in this range are recorded in `splits.txt` as comments and NOT claimed: a stub object
 * emits nothing, and a range our object does not emit must not be claimed (playbook 23, docs/plan.md
 * 8.4).  The extab/extabindex fragments already travel with the code unit.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8014A1BC_fn_8014A1BC.c`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80149D6C.h"
#include "enemy/fn_80147CE0.h"
#include "unsplit/enemy.h"

/* ---------------------------------------------------------------------------------------------------
 * nw4r math types
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` (three `f32` at +0x00/+0x04/+0x08) comes from `nw4r/math.h`, which is includable from C. */
/* ---------------------------------------------------------------------------------------------------
 * the enemy work record
 * ------------------------------------------------------------------------------------------------- */

/* size: 0xA80 (lower bound: the highest field this unit touches is +0xA7E) */
typedef struct _ENEMY_WORK {
    /* +0x00 */ u8 pad_0x00[0x03];
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 pad_0x04;
    /* +0x05 */ u8 state;
    /* +0x06 */ u8 sub_state;
    /* +0x07 */ u8 step_count;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u8 pad_0x24[0x164];
    /* +0x188 */ VEC3 v_0x188;
    /* +0x194 */ u8 pad_0x194[0x18];
    /* +0x1AC */ f32 field_0x1ac;
    /* +0x1B0 */ VEC3 v_0x1b0;
    /* +0x1BC */ u32 field_0x1bc;
    /* +0x1C0 */ u32 field_0x1c0;
    /* +0x1C4 */ u8 pad_0x1c4[0x1D];
    /* +0x1E1 */ u8 field_0x1e1;
    /* +0x1E2 */ u8 pad_0x1e2[0x04];
    /* +0x1E6 */ u8 field_0x1e6;
    /* +0x1E7 */ u8 field_0x1e7;
    /* +0x1E8 */ u8 pad_0x1e8[0x24];
    /* +0x20C */ f32 field_0x20c;
    /* +0x210 */ u8 pad_0x210[0x04];
    /* +0x214 */ f32 field_0x214;
    /* +0x218 */ u8 pad_0x218[0xF8];
    /* +0x310 */ VEC3 v_0x310;
    /* +0x31C */ u8 pad_0x31c[0x04];
    /* +0x320 */ VEC3 v_0x320;
    /* +0x32C */ u16 field_0x32c;
    /* +0x32E */ u8 pad_0x32e[0x3E];
    /* +0x36C */ VEC3 v_0x36c;
    /* +0x378 */ f32 field_0x378;
    /* +0x37C */ u32 field_0x37c;
    /* +0x380 */ u8 pad_0x380[0x02];
    /* +0x382 */ u8 field_0x382;
    /* +0x383 */ u8 pad_0x383[0x421];
    /* +0x7A4 */ u32 field_0x7a4;
    /* +0x7A8 */ u8 pad_0x7a8[0x20];
    /* +0x7C8 */ u8 field_0x7c8;
    /* +0x7C9 */ u8 pad_0x7c9[0x2A0];
    /* +0xA69 */ u8 field_0xa69;
    /* +0xA6A */ u8 pad_0xa6a[0x14];
    /* +0xA7E */ u16 field_0xa7e;
    /* +0xA80 */ u8 pad_0xa80[0x00];
} _ENEMY_WORK;

/* size: 0x18 (lower bound: the unit reads +0x12 and +0x14) */
typedef struct ShellParams {
    /* +0x00 */ u8 pad_0x00[0x12];
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
    /* +0x16 */ u8 pad_0x16[0x02];
} ShellParams;

/* The shell callback table; only the entry at +0x3C is used by this unit. size: 0x40 */
typedef struct ShellSetFunc {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ void (*field_0x3c)(_ENEMY_WORK *self, void *params, u32 mode, void *table);
} ShellSetFunc;

extern ShellSetFunc *shell_set_func_ptr;

/* size: 0x10 (lower bound: this unit reads +0x0C) */
typedef struct SystemWork {
    /* +0x00 */ u32 pad_0x00[3];
    /* +0x0C */ u32 field_0x0c;
} SystemWork;

extern SystemWork system_w;

/* ---------------------------------------------------------------------------------------------------
 * callees and pool literals owned by other units (declared by their map spelling; playbook 29)
 * ------------------------------------------------------------------------------------------------- */

extern f32 lbl_80796EEC;
extern f32 lbl_80796EF0;
extern f32 lbl_80796EF4;
extern f32 lbl_805A1CC8[];

extern void fn_8012CF20();
extern f32 fn_802B0430(u8 id);
extern void fn_80127FE4(_ENEMY_WORK *self);

/* ---------------------------------------------------------------------------------------------------
 * fn_8014A1BC - advance the object's two-level state machine and gate one timer-driven action
 * ------------------------------------------------------------------------------------------------- */

extern f32 lbl_80796E1C;
extern f32 lbl_80796E58;
extern f32 lbl_80796EF8;
extern f32 lbl_80796EFC;
extern f32 lbl_80796F00;
extern f32 lbl_80796F04;
extern f32 lbl_80796F08;
extern f32 lbl_80796F0C;
extern f32 lbl_80796F10;
extern f32 lbl_80796F14;
extern void fn_80134004(_ENEMY_WORK *self, u32 a, f32 b);
extern void fn_801280AC(_ENEMY_WORK *self);
extern f32 lbl_80796F18;
extern void fn_80154CA4(_ENEMY_WORK *self);
extern f32 lbl_80796EBC;
extern f32 lbl_80796F1C;
extern f32 lbl_80796E20;
extern f32 lbl_80796EA8;
extern u32 lbl_8056F9A0[];
extern u32 lbl_8056FA0C[];
extern f32 lbl_805A1D88[];
extern f32 lbl_805A1FF0[];
extern f32 lbl_805A2028[];
extern f32 lbl_80796E38;
extern f32 lbl_80796E3C;
extern f32 lbl_80796E4C;
extern f32 lbl_80796ED8;
extern f32 lbl_80796F20;
extern f32 lbl_80796F24;
extern f32 lbl_80796F28;
extern f32 lbl_80796F2C;
extern void fn_80043EA8(void *p);
extern void fn_80056A54(_ENEMY_WORK *self, u32 a, u32 b);
extern void setVector3__FPQ34nw4r4math4VEC3fff(VEC3 *v, f32 x, f32 y, f32 z);
extern void fn_80304508(_ENEMY_WORK *self, u32 a, u32 b, VEC3 *v, f32 s);
extern void fn_80129668(_ENEMY_WORK *self, u32 a, u32 b);
extern f32 lbl_80796E0C;
extern f32 lbl_80796E34;
extern f32 lbl_80796E60;
extern f32 lbl_80796F30;
extern f32 lbl_80796F34;
extern f32 lbl_80796F38;
extern f32 lbl_80796F3C;
extern void fn_80154C74(_ENEMY_WORK *self);
extern void fn_8012933C(_ENEMY_WORK *self, u32 a, u32 b, u32 c);
extern void fn_80129724(_ENEMY_WORK *self, u32 a);

extern f32 lbl_805A1D18[];
extern f32 lbl_805A1D40[];
extern f32 lbl_805A1DD8[];
extern f32 lbl_805A1E20[];
extern f32 lbl_805A1E50[];
extern f32 lbl_805A1E90[];
extern f32 lbl_805A1F00[];
extern f32 lbl_80796E24;
extern f32 lbl_80796EA4;
extern f32 lbl_80796F60;
extern f32 lbl_80796F74;
extern f32 lbl_80796F78;
extern f32 lbl_80796F7C;
extern f32 lbl_80796FD4;
extern f32 lbl_80796FD8;
extern void fn_80127F48();
extern void fn_8012B380();

extern f32 lbl_805A2AA0[];
extern f32 lbl_805A2B00[];
extern f32 lbl_805A2B50[];
extern f32 lbl_805A2B98[];
extern f32 lbl_805A2BC8[];
extern f32 lbl_80796F84;
extern f32 lbl_80796F9C;
extern f32 lbl_80796FA0;
extern void fn_8011E6EC();
extern void fn_801251D8();
extern u32 fn_8012EC3C();
extern u32 fn_80154638();
extern void fn_802B9574();
extern void fn_803B9BA0();

extern f32 lbl_80796FA4;
extern f32 lbl_80796FA8;

extern f32 lbl_805A2338[];
extern f32 lbl_805A2360[];
extern f32 lbl_805A2388[];
extern f32 lbl_805A23E0[];
extern f32 lbl_805A2450[];
extern f32 lbl_805A2490[];
extern f32 lbl_805A2518[];
extern f32 lbl_805A2588[];
extern f32 lbl_805A25B0[];
extern f32 lbl_805A25F0[];
extern f32 lbl_805A2620[];
extern f32 lbl_805A2678[];
extern f32 lbl_805A26E0[];
extern f32 lbl_805A2718[];
extern f32 lbl_805A27A0[];
extern f32 lbl_805A2808[];
extern f32 lbl_805A2830[];
extern f32 lbl_805A2858[];
extern f32 lbl_805A2880[];
extern f32 lbl_805A28F0[];
extern f32 lbl_805A2950[];
extern f32 lbl_805A2978[];
extern f32 lbl_805A29A0[];
extern f32 lbl_805A29E0[];
extern f32 lbl_805A2A58[];
extern void fn_801251D0();

extern f32 lbl_80796E2C;
extern f32 lbl_80796E50;
extern f32 lbl_80796E54;
extern f32 lbl_80796E5C;
extern f32 lbl_80796E64;
extern f32 lbl_80796E68;
extern f32 lbl_80796E6C;
extern f32 lbl_80796E70;
extern f32 lbl_80796E74;
extern f32 lbl_80796E78;
extern f32 lbl_80796E7C;
extern f32 lbl_80796E80;
extern f32 lbl_80796E84;
extern f32 lbl_80796E88;
extern f32 lbl_80796E8C;
extern f32 lbl_80796E90;
extern f32 lbl_80796E94;
extern f32 lbl_80796E98;
extern f32 lbl_80796E9C;
extern f32 lbl_80796F6C;
extern f32 lbl_80796F70;
extern void fn_80041E40();
extern void fn_80050CA0();

extern f32 lbl_8056F960[];
extern f32 lbl_80796F94;
extern f32 lbl_80796F98;
extern f32 fn_80050EAC();
extern void fn_80051378();
extern void fn_80051EE0();
extern void fn_80128BF8();
extern void fn_80154B04();


extern f32 lbl_80796F8C;
extern f32 lbl_80796F90;
extern void fn_801545B8();

extern f32 lbl_805A1F98[];
extern f32 lbl_805A2078[];
extern f32 lbl_805A20A0[];
extern f32 lbl_80796F40;
extern f32 lbl_80796F44;
extern f32 lbl_80796F48;
extern f32 lbl_80796F5C;
extern f32 lbl_80796F80;


extern f32 lbl_80796F88;
extern f32 fn_80050F80();
extern void fn_800AD9C0();


extern f32 lbl_80796ECC;
extern f32 lbl_80796FAC;
extern f32 lbl_80796FB0;
extern f32 lbl_80796FB4;


void fn_8014A1BC(_ENEMY_WORK *self, s32 arg1, s32 arg2);
void fn_8014A334(_ENEMY_WORK *self, s32 arg1);
void fn_8014A418(_ENEMY_WORK *self);
void fn_8014A7B0(_ENEMY_WORK *self, s32 arg1);
void fn_8014A860(_ENEMY_WORK *self);
void fn_8014A8EC(_ENEMY_WORK *self, s32 arg1);
void fn_8014AA4C(_ENEMY_WORK *self);
void fn_8014AB24(_ENEMY_WORK *self);
void fn_8014ABEC(_ENEMY_WORK *self);
void fn_8014ACAC(_ENEMY_WORK *self);
void fn_8014ADC8(_ENEMY_WORK *self);
void fn_8014AEAC(_ENEMY_WORK *self);
void fn_8014AF44(_ENEMY_WORK *self);
void fn_8014B020(_ENEMY_WORK *self);
void fn_8014B0E8(_ENEMY_WORK *self);
void fn_8014B1F8(_ENEMY_WORK *self, s32 arg1);
void fn_8014B378(_ENEMY_WORK *self);
void fn_8014B4EC(_ENEMY_WORK *self, s32 arg1);
void fn_8014B5C0(_ENEMY_WORK *self);
void fn_8014B630(_ENEMY_WORK *self);
void fn_8014B6CC(_ENEMY_WORK *self, s32 arg1);
void fn_8014BDAC(_ENEMY_WORK *a0, s32 arg1, s32 arg2, s32 arg3);
void fn_8014BDF8(_ENEMY_WORK *self, s32 arg1);
void fn_8014BFE4(_ENEMY_WORK *self, s32 arg1);
void fn_8014C16C(_ENEMY_WORK *self, s32 arg1);
void fn_8014CBA0(_ENEMY_WORK *self);
void fn_8014CC98(_ENEMY_WORK *self);
void fn_8014CDD0(_ENEMY_WORK *self, s32 arg1);
void fn_8014C6DC(_ENEMY_WORK *self, u8 arg1);
void fn_8014D504(_ENEMY_WORK *self);
void fn_8014D714(_ENEMY_WORK *self);
void fn_8014E160(_ENEMY_WORK *self);
void fn_8014E3AC(_ENEMY_WORK *self);
void fn_8014EA70(_ENEMY_WORK *self);
void fn_8014EA84(_ENEMY_WORK *self);
void fn_8014EC50(_ENEMY_WORK *self);
void fn_8014EDC8(_ENEMY_WORK *self);
void fn_8014F010(void);
void fn_8014F020(void);
void fn_8014F030(_ENEMY_WORK *self);
void fn_8014F530(_ENEMY_WORK *self);
void fn_8014F5D8(void);
void fn_8014F5DC(_ENEMY_WORK *self);
void fn_8014F5F0(_ENEMY_WORK *self);
void fn_8014FA34(_ENEMY_WORK *self);
void fn_8014FE00(_ENEMY_WORK *self);
void fn_8014F138(_ENEMY_WORK *self);
void fn_8014F078(_ENEMY_WORK *self);
void fn_8014FF10(_ENEMY_WORK *self);
void fn_8014CEF8(_ENEMY_WORK *self, u8 arg1, u8 arg2);
u32 fn_8014E670(_ENEMY_WORK *self, u8 arg1);
void fn_8014FC24(_ENEMY_WORK *self);
void fn_8014E290(_ENEMY_WORK *self);
void fn_8014BB40(_ENEMY_WORK *self, u8 arg1);
void fn_8014C464(_ENEMY_WORK *self, u8 arg1);
void fn_8014D8F0(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void fn_8014DAF4(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void fn_8014DDEC(_ENEMY_WORK *self);
void fn_8014E774(_ENEMY_WORK *self);
void fn_8014F71C(_ENEMY_WORK *self);

void fn_8014A1BC(_ENEMY_WORK *self, s32 arg1, s32 arg2) {
    f32 temp_f2;
    f32 var_f3;
    u8 temp_r0;
    u8 temp_r3;

    if ((arg2 & 0xFF) == 1) {
        fn_8012CF20();
    }
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0U;
        self->step_count = 0U;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        fn_801353E4(self);
        return;
    case 1:
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {
        case 0:
            self->v_0x310.y = fn_80135644(self, &lbl_805A1CC8);
            fn_80135418(self);
            if (fn_8012F93C(self) == 1U) {
                if (++self->step_count >= 2U) {
                    self->sub_state = (u8) (self->sub_state + 1);
                    fn_80134DF4(self);
                }
            }
            break;
        case 1:
            fn_80134E28(self);
            break;
        }
        if ((arg1 & 0xFF) != 1) {
            var_f3 = fn_802B0430(self->field_0x1e1) - lbl_80796EEC;
            temp_f2 = self->field_0x20c;
            if ((var_f3 - temp_f2) < lbl_80796EF0) {
                var_f3 = lbl_80796EF0 + temp_f2;
            }
        } else {
            var_f3 = lbl_80796EF4 + self->v_0x36c.y;
        }
        if (self->v_0x188.y >= var_f3) {
            fn_80127FE4(self);
        }
        return;
    }
}



/* ---------------------------------------------------------------------------------------------------
 * fn_8014A334 - second-level state machine: advance to the arrival substate on a frame check
 * ------------------------------------------------------------------------------------------------- */

void fn_8014A334(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    if ((u8) arg1 == 1) {
        fn_8012CF20();
    }
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        break;
    case 1:
        if (fn_80133C50(self, 0x200) == 1U) {
            if ((arg1 & 0xFF) != 2) {
                if ((arg1 & 0xFF) != 3) {
                    fn_80127FE4(self);
                    break;
                }
                fn_80128A14(self, 3, 0x15);
                break;
            }
            fn_80128A14(self, 3, 0x14);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014A418 - the charge/scale walk: step through the approach substates and scale the offset
 * ------------------------------------------------------------------------------------------------- */

void fn_8014A418(_ENEMY_WORK *self) {
    u32 var_r30;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    fn_8012CF20();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x2D, 6, 0);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0x3D, 6, 0);
            self->v_0x36c.y = self->v_0x36c.y + (lbl_80796EF8 * get_em_chg_scale__FP11_ENEMY_WORK(self));
            self->sub_state = 0U;
            return;
        }
        return;
    case 2:
        fn_80133C3C(self);
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {
        case 0:
            fn_80133CC8(self, 0x100, 0x100);
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
                fn_801353F8(self);
                self->v_0x310.z = lbl_80796F00;
                self->v_0x320.y = lbl_80796F04;
                fn_80134004(self, 0x10, lbl_80796F08);
            }
            break;
        case 1:
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1bc);
            fn_80134114(self, 0, 0x80U);
            if (self->v_0x310.z > lbl_80796E58) {
                self->v_0x310.z = lbl_80796E58;
            }
            break;
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0x3E, 6, 0);
            return;
        }
        break;
    case 3:
        fn_80133C3C(self);
        if (fn_80134114(self, 0, 0x80U) == 1) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0x2D, 6, 0);
            self->sub_state = 0U;
            self->v_0x310.y = lbl_80796F0C;
            self->v_0x320.y = lbl_80796F10;
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1bc);
            return;
        }
        fn_80130248(self);
        fn_80135600(self, &self->field_0x1bc);
        if (self->v_0x310.z > lbl_80796E58) {
            self->v_0x310.z = lbl_80796E58;
            return;
        }
        break;
    case 4:
        temp_r0_2 = self->sub_state;
        switch ((s32) temp_r0_2) {
        case 0:
            fn_80130248(self);
            var_r30 = fn_80135600(self, &self->field_0x1bc);
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
                self->v_0x320.y = lbl_80796F14;
            }
            break;
        case 1:
            fn_80130248(self);
            var_r30 = fn_80135600(self, &self->field_0x1bc);
            if (self->v_0x310.z < lbl_80796E1C) {
                self->v_0x310.z = lbl_80796E1C;
            }
            break;
        }
        if (fn_8012F93C(self) == 1U) {
            if (var_r30 == 1U) {
                self->state = (u8) (self->state + 1);
                fn_80130478(self, 3);
                fn_8012F5B8(self, 0x1D, 6, 0);
                return;
            }
            if (fn_8012D1A0(self) == 1U) {
                fn_80127FE4(self);
                return;
            }
        }
        break;
    case 5:
        if (fn_8012F93C(self) == 1U) {
            fn_80130478(self, 3);
            fn_801280AC(self);
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014A7B0 - the short two-state variant: arm a hit check, then run the frame check
 * ------------------------------------------------------------------------------------------------- */

void fn_8014A7B0(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    fn_8012CF20();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x2D, 6, 0);
        break;
    case 1:
        if ((arg1 & 0xFF) == 1) {
            fn_80133C50(self, 0x180);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014A860 - a plain countdown state: arm a 60-frame timer, then fire when it runs out
 * ------------------------------------------------------------------------------------------------- */

void fn_8014A860(_ENEMY_WORK *self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1F, 4, 0);
        self->field_0x20 = 0x3C;
        break;
    case 1:
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            fn_801481FC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014A8EC - the arrival substate machine: wait for the frame check, then hand off to the caller
 * ------------------------------------------------------------------------------------------------- */

void fn_8014A8EC(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r0;
    u8 temp_r3;

    fn_8012CF20();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0U;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x3D, 6, 0);
        break;
    case 1:
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {
        case 0:
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796E58, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
                fn_801353F8(self);
                self->v_0x310.z = lbl_80796F00;
                self->v_0x320.y = lbl_80796F04;
            }
            break;
        case 1:
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1bc);
            if (self->v_0x310.z > lbl_80796F18) {
                self->v_0x310.z = lbl_80796F18;
            }
            break;
        }
        if (fn_8012F93C(self) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    fn_801481FC(self);
                    break;
                }
                fn_80128A14(self, 3, 0x17);
                break;
            }
            fn_80128A14(self, 3, 0x16);
            break;
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014AA4C - the state-0x1E6 dispatcher: tail-call the handler for the current action code
 * ------------------------------------------------------------------------------------------------- */

void fn_8014AA4C(_ENEMY_WORK *self) {
    switch (self->field_0x1e6) {
    case 0: fn_801498C8(self); break;
    case 1: fn_80149A08(self); break;
    case 2: fn_80149AFC(self); break;
    case 3: fn_80149C54(self); break;
    case 4: fn_80149C58(self); break;
    case 5: fn_80149D6C(self, 0); break;
    case 6: fn_8014A1BC(self, 0, 0); break;
    case 7: fn_8014A334(self, 0); break;
    case 8: fn_8014A418(self); break;
    case 9: fn_80149D6C(self, 1); break;
    case 10: fn_8014A7B0(self, 0); break;
    case 11: fn_8014A860(self); break;
    case 12: fn_8014A8EC(self, 0); break;
    case 13: fn_8014A1BC(self, 1, 0); break;
    case 14: fn_80149D6C(self, 2); break;
    case 15: fn_8014A7B0(self, 1); break;
    case 16: fn_8014A1BC(self, 0, 1); break;
    case 17: fn_8014A334(self, 1); break;
    case 18: fn_8014A334(self, 2); break;
    case 19: fn_8014A334(self, 3); break;
    case 20: fn_8014A8EC(self, 1); break;
    case 21: fn_8014A8EC(self, 2); break;
    case 22: fn_80149D6C(self, 3); break;
    case 23: fn_80149D6C(self, 4); break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014AB24 - a three-state handoff: mark the action, wait for the frame check, then relink
 * ------------------------------------------------------------------------------------------------- */

void fn_8014AB24(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x2A, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x44, 0, 0);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014ABEC - the same three-state shape as fn_8014AB24, ending in the shared post-action handler
 * ------------------------------------------------------------------------------------------------- */

void fn_8014ABEC(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x2E, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x41, 0, 0);
            return;
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014ACAC - hold the current pose until the frame check, then fade out or return to the base pose
 * ------------------------------------------------------------------------------------------------- */

void fn_8014ACAC(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1E, 6, 0);
        break;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130248(self);
            fn_801305C4(self);
            fn_801353F8(self);
            self->v_0x310.y = lbl_80796F00 * fn_8012F8E4(self);
            self->v_0x320.x = lbl_80796F1C * fn_8012F8E4(self);
            return;
        }
        return;
    case 2:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EBC, lbl_80796E1C) == 0) {
            fn_80135418(self);
        } else {
            CancelFade(self);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127FE4(self);
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014ADC8 - run a one-shot transition, then hand the finished state to the shared post-action
 * ------------------------------------------------------------------------------------------------- */

void fn_8014ADC8(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        fn_80134E8C(self);
        break;
    case 1:
        fn_80134F18(self);
        fn_80130248(self);
        if (fn_80130008(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x1D, 6, 0);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014AEAC - play the shared animation table, then hand the finished state to the post-action
 * ------------------------------------------------------------------------------------------------- */

void fn_8014AEAC(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80134964(self, &lbl_8056F9A0, 0, 0, 0U);
        break;
    case 1:
        if (fn_80134B0C(self, &lbl_8056F9A0) == 1U) {
            fn_801280AC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014AF44 - arm the approach scale and push the offset to its limit while the frame check runs
 * ------------------------------------------------------------------------------------------------- */

void fn_8014AF44(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1B, 4, 0);
        fn_80134004(self, 0, lbl_80796F08);
        fn_801353F8(self);
        self->v_0x310.z = lbl_80796EA8;
        self->v_0x320.y = lbl_80796E20;
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80U) == 1) {
            fn_801280AC(self);
            return;
        }
        fn_801355C8(self, &self->field_0x1bc);
        if (self->v_0x310.z > lbl_80796E58) {
            self->v_0x310.z = lbl_80796E58;
        }
        return;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014B020 - play the stance table until the frame check, then hand off to the post-action
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B020(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80134F70(self, &lbl_8056FA0C);
        fn_80134004(self, 9, lbl_80796E1C);
        break;
    case 1:
        if (fn_80134114(self, 0, 0U) == 1) {
            fn_801280AC(self);
            return;
        }
        fn_80135000(self, 0, &lbl_8056FA0C);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1bc);
        return;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014B0E8 - arm the pivot action, then hold the offset and let the frame check finish it
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B0E8(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x21, 4, 0);
        fn_801353E4(self);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x42, 0, 0);
            self->v_0x310.z = fn_80135644(self, &lbl_805A1D88);
            fn_801354F4(self, &self->field_0x1bc);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        self->v_0x310.z = fn_80135644(self, &lbl_805A1D88);
        fn_801354F4(self, &self->field_0x1bc);
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014B1F8 - the mirrored strafe: pick the turn direction from the argument and scale the offset
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B1F8(_ENEMY_WORK *self, s32 arg1) {
    s32 sp10;
    s32 spC;
    s32 sp8;
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        if (arg1 == 0) {
            fn_8012F5B8(self, 0xCE, 6, 0);
        } else {
            fn_8012F5B8(self, 0xCF, 6, 0);
        }
        fn_801353E4(self);
        self->field_0x37c = self->field_0x1c0;
        self->v_0x320.z = fn_801356A8(self, lbl_80796E20, lbl_80796F20, lbl_80796F24);
        break;
    case 1:
        if (arg1 == 0) {
            fn_80133E3C(self, -0x4000, lbl_80796ED8, lbl_80796F28);
            self->v_0x310.x = fn_80135644(self, &lbl_805A1FF0);
        } else {
            fn_80133E3C(self, 0x4000, lbl_80796ED8, lbl_80796F28);
            self->v_0x310.x = -fn_80135644(self, &lbl_805A1FF0);
        }
        self->v_0x310.z = self->v_0x320.z * fn_80135644(self, &lbl_805A2028);
        sp8 = 0;
        spC = self->field_0x37c;
        sp10 = 0;
        fn_801354F4(self, &sp8);
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014B378 - spark the two-stage hit effect while the action runs, at most once every eight frames
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B378(_ENEMY_WORK *self) {
    VEC3 sp8;
    u8 temp_r3;

    fn_80043EA8(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1C, 6, 0);
        self->field_0x20 = 0;
        break;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F18, lbl_80796E1C) == 1U) {
            fn_8012933C(self, 0, 8, 5);
            fn_80056A54(self, 0x1B, 0xA);
        }
        setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F18, lbl_80796E1C) == 1U) {
            fn_80304508(self, 0, 0x1A, &sp8, lbl_80796E20);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796E4C, lbl_80796F2C) == 1U) {
            if ((self->field_0x20 & 7) == 0) {
                fn_80304508(self, 1, 0x1A, &sp8, lbl_80796E20);
            }
            self->field_0x20 = self->field_0x20 + 1;
        }
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        return;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014B4EC - count a fixed number of substeps (three or six, by the argument) before finishing
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B4EC(_ENEMY_WORK *self, s32 arg1) {
    VEC3 sp8;
    u32 var_r31;
    u8 temp_r0;
    u8 temp_r3;

    fn_80043EA8(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0U;
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1B, 6, 0);
        break;
    case 1:
        var_r31 = 6U;
        if (arg1 == 0) {
            var_r31 = 3U;
        }
        if (fn_8012F93C(self) == 1U) {
            temp_r0 = self->sub_state + 1;
            self->sub_state = temp_r0;
            if (temp_r0 >= var_r31) {
                fn_801280AC(self);
            }
        }
        return;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014B5C0 - the state-0x1E6 dispatcher for the second action group: tail-call the handler
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B5C0(_ENEMY_WORK *self) {
    switch (self->field_0x1e6) {
    case 0: fn_8014AB24(self); break;
    case 1: fn_8014ABEC(self); break;
    case 2: fn_8014ACAC(self); break;
    case 3: fn_8014ADC8(self); break;
    case 4: fn_8014AEAC(self); break;
    case 5: fn_8014AF44(self); break;
    case 6: fn_8014B020(self); break;
    case 7: fn_8014B0E8(self); break;
    case 8: fn_8014B1F8(self, 0); break;
    case 9: fn_8014B1F8(self, 1); break;
    case 10: fn_8014B378(self); break;
    case 11: fn_8014B378(self); break;
    case 12: fn_8014B4EC(self, 0); break;
    case 13: fn_8014B4EC(self, 1); break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014B630 - arm the two sub-part motions and run the shared post-action on the frame check
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B630(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x2F, 4, 0);
        fn_80129668(self, 0, 1);
        fn_80129668(self, 1, 9);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * fn_8014B6CC - the long action: approach, hold and release the scale over four outer states
 * ------------------------------------------------------------------------------------------------- */

void fn_8014B6CC(_ENEMY_WORK *self, s32 arg1) {
    s32 temp_r0_2;
    s32 temp_r0_4;
    s32 temp_r4;
    s32 temp_r4_2;
    u8 temp_r0;
    u8 temp_r0_3;
    u8 temp_r0_5;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        self->sub_state = 0U;
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xD1, 6, 0);
        self->field_0x20 = 0;
        return;
    case 1:
        fn_801303FC(self, lbl_80796F30);
        if (self->field_0x1ac < lbl_80796F34) {
            self->field_0x1ac = lbl_80796F34;
        }
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {
        case 0:
            fn_80133C50(self, 0x100);
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796E60, lbl_80796E1C) != 1U) {
                return;
            }
            self->sub_state = (u8) (self->sub_state + 1);
            fn_801353F8(self);
            self->v_0x320.y = lbl_80796F04;
            fn_80134004(self, 0x10, lbl_80796E1C);
            fn_8012933C(self, 0, 0xE, 3);
            return;
        case 1:
            fn_801355C8(self, &self->field_0x1bc);
            if (self->v_0x310.z > lbl_80796E34) {
                self->v_0x310.z = lbl_80796E34;
            }
            if (fn_8012F93C(self) == 1U) {
                self->state = (u8) (self->state + 1);
                fn_8012F5B8(self, 0xD0, 6, 0);
                return;
            }
            break;
        }
        break;
    case 2:
        fn_801303EC(self, lbl_80796F34);
        fn_801355C8(self, &self->field_0x1bc);
        if (self->v_0x310.z > lbl_80796E34) {
            self->v_0x310.z = lbl_80796E34;
        }
        if ((u8) fn_80134114(self, 0, 0U) == 1) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            switch (arg1 & 0xFF) {
            default:
                fn_8012F5B8(self, 0xD2, 6, 0);
                self->v_0x320.y = lbl_80796F38;
                self->field_0x20 = 0;
                return;
            case 1:
                fn_80130478(self, 0);
                fn_8012F5B8(self, 0xDD, 0, 0);
                fn_801303EC(self, lbl_80796E1C);
                return;
            case 2:
                fn_8012F5B8(self, 0x35, 0xA, 0);
                return;
            }
        }
        break;
    case 3:
        switch (arg1 & 0xFF) {
        default:
            temp_r4 = self->field_0x20;
            if (temp_r4 < 0x14) {
                temp_r0_2 = temp_r4 + 1;
                self->field_0x20 = temp_r0_2;
                fn_801303EC(self, (lbl_80796F3C * (f32) (0x14 - temp_r0_2)) / lbl_80796E0C);
            }
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                fn_80129724(self, 0);
            }
            fn_801355C8(self, &self->field_0x1bc);
            if (self->v_0x310.z < lbl_80796E1C) {
                self->v_0x310.z = lbl_80796E1C;
            }
            if (fn_8012F93C(self) == 1U) {
                fn_801280AC(self);
                return;
            }
            break;
        case 1:
            temp_r0_5 = self->sub_state;
            switch ((s32) temp_r0_5) {
            case 0:
                if (fn_8012F93C(self) == 1U) {
                    self->sub_state = (u8) (self->sub_state + 1);
                    fn_8012F5B8(self, 0xDA, 0, 0);
                    return;
                }
                break;
            case 1:
                if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    fn_80129724(self, 0);
                }
                if (fn_8012F93C(self) == 1U) {
                    fn_80127F48(self);
                    return;
                }
                break;
            }
            break;
        case 2:
            temp_r0_3 = self->sub_state;
            switch ((s32) temp_r0_3) {
            case 0:
                temp_r4_2 = self->field_0x20;
                if (temp_r4_2 < 0xA) {
                    temp_r0_4 = temp_r4_2 + 1;
                    self->field_0x20 = temp_r0_4;
                    fn_801303EC(self, (lbl_80796F34 * (f32) (5 - temp_r0_4)) / lbl_80796F00);
                }
                if (fn_8012F93C(self) == 1U) {
                    self->sub_state = (u8) (self->sub_state + 1);
                    fn_80129724(self, 0);
                    fn_80130478(self, 0);
                    fn_8012F5B8(self, 0x3C, 0, 0);
                    return;
                }
                break;
            case 1:
                if (fn_8012F93C(self) == 1U) {
                    fn_80127F48(self);
                }
                break;
            }
            break;
        }
        break;
    }
}

extern f32 lbl_80796F4C;

/* ---------------------------------------------------------------------------------------------------
 * fn_8014BDAC - pick one of four action ids from two flags and hand it to the action starter
 * ------------------------------------------------------------------------------------------------- */

void fn_8014BDAC(_ENEMY_WORK *a0, s32 arg1, s32 arg2, s32 arg3) {
    u8 var_r0;

    {
        u8 a2 = arg2;
        u8 a3 = arg3;
        if (a2 == 0) {
            if (a3 == 0) {
                var_r0 = 1;
            } else {
                var_r0 = 3;
            }
        } else {
            var_r0 = 4;
            if (a3 == 0) {
                var_r0 = 2;
            }
        }
    }
    fn_801493A8(a0, arg1, var_r0, 0);
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014BDF8 - the two-motion grab: pick the motion ids from the argument and run both sub-motions
 * ------------------------------------------------------------------------------------------------- */

void fn_8014BDF8(_ENEMY_WORK *self, s32 arg1) {
    u32 var_r27;
    u32 var_r28;
    u32 var_r29;
    u32 var_r30;
    u8 temp_r3;
    u8 temp_r3_2;
    u8 temp_r5;

    if ((arg1 & 0xFF) != 1) {
        var_r30 = 0x32;
        var_r29 = 0x1D;
        var_r28 = 0x22;
        var_r27 = 0x23;
    } else {
        var_r30 = 0x31;
        var_r29 = 4;
        var_r28 = 0x20;
        var_r27 = 0x21;
    }
    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        self->sub_state = 0U;
        fn_80130478(self, 0);
        fn_8012F5B8(self, var_r30, 4, 0);
        fn_8012933C(self, 0, var_r29, 8);
        fn_8012933C(self, 1, var_r28, 0x18);
        break;
    case 1:
        fn_80133F4C(self, lbl_80796E1C, lbl_80796F28);
        temp_r3 = self->sub_state;
        if ((temp_r3 == 0) && (self->field_0xa69 == 0)) {
            self->sub_state = (u8) (temp_r3 + 1);
            fn_8012933C(self, 1, var_r27, 0x10);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_8012F5B8(self, var_r30, 0, 0);
            fn_8012933C(self, 0, var_r29, 8);
            fn_8012933C(self, 1, var_r28, 0x18);
            return;
        }
        return;
    case 2:
        fn_80133F4C(self, lbl_80796E1C, lbl_80796F28);
        temp_r3_2 = self->sub_state;
        if ((temp_r3_2 == 0) && (self->field_0xa69 == 0)) {
            self->sub_state = (u8) (temp_r3_2 + 1);
            fn_8012933C(self, 1, var_r27, 0x10);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014BFE4 - the approach-and-turn action: drive the two scale motions and hand off when done
 * ------------------------------------------------------------------------------------------------- */

void fn_8014BFE4(_ENEMY_WORK *self, s32 arg1) {
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xCE, 0xA, 0);
        fn_80129668(self, 0, 0x1B);
        break;
    case 1:
        fn_8012CF20();
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            if (arg1 == 0) {
                fn_80130478(self, 0);
                fn_8012F5B8(self, 0xCF, 0, 0);
                fn_80129668(self, 0, 0x16);
                return;
            }
            fn_80130478(self, 3);
            fn_801280AC(self);
            return;
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F4C, lbl_80796E1C) == 1U) {
            temp_f31 = fn_8012F8F4(self) - lbl_80796F4C;
            fn_801303EC(self, lbl_80796F3C * ((fn_8012F8EC(self) - lbl_80796F4C) / temp_f31));
            return;
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

extern f32 lbl_80796EA0;
extern f32 lbl_80796ED8;
extern f32 lbl_80796F50;
extern f32 lbl_80796F54;
extern f32 lbl_80796F58;
extern f32 lbl_80796F64;
extern f32 lbl_80796F68;
extern f32 fn_80050EF4(void *a, void *b);
extern void fn_80128A70(_ENEMY_WORK *self, u32 a, u32 b);

/* ---------------------------------------------------------------------------------------------------
 * fn_8014C16C - the entry action: pick a scale profile from the argument, then run the frame checks
 * ------------------------------------------------------------------------------------------------- */

void fn_8014C16C(_ENEMY_WORK *self, s32 arg1) {
    s32 temp_r0;
    u16 var_r5;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        fn_80130478(self, 0);
        fn_80154C74(self);
        fn_8012F5B8(self, 8, 4, 0);
        fn_8012933C(self, 0, 5, 3);
        self->field_0x20 = 0;
        switch (arg1) {
        case 0:
            fn_80134004(self, 0x10, lbl_80796F08);
            self->field_0x20 = 0x14;
            return;
        case 1:
            fn_80134004(self, 0x10, lbl_80796F50);
            self->field_0x20 = 0xA;
            return;
        case 2:
            fn_80134004(self, 0, lbl_80796F08);
            if (self->field_0x378 == lbl_80796EEC) {
                self->field_0x378 = lbl_80796EEC;
                return;
            }
            return;
        default:
            fn_80134004(self, 0, lbl_80796F08);
            return;
        case 4:
            fn_80134004(self, 0x10, lbl_80796F54);
            return;
        }
        break;
    case 1:
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            self->field_0x20 = 0;
        }
        var_r5 = 0x40;
        if (arg1 == 0) {
            var_r5 = 0x80;
        }
        if ((fn_80134114(self, 0, var_r5) == 1) && (self->field_0x20 <= 0)) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            switch (arg1) {
            default:
                fn_80129724(self, 0);
                fn_8012F5B8(self, 9, 4, 0);
                return;
            case 2:
                fn_8012F5B8(self, 0x40, 4, 0);
                return;
            case 3:
                fn_8012F5B8(self, 0x30, 4, 0);
                return;
            case 4:
                fn_80129724(self, 0);
                fn_80128A70(self, 4, 7);
                return;
            }
        }
        break;
    case 2:
        switch (arg1) {
        case 2:
            if ((self->sub_state == 0) && (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F28, lbl_80796E1C) == 1U)) {
                self->sub_state = (u8) (self->sub_state + 1);
                fn_80129724(self, 0);
            }
            break;
        case 3:
            if ((self->sub_state == 0) && (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F58, lbl_80796E1C) == 1U)) {
                self->sub_state = (u8) (self->sub_state + 1);
                fn_80129668(self, 0, 0xD);
            }
            break;
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014CBA0 - arm the short stance and finish it on the frame check
 * ------------------------------------------------------------------------------------------------- */

void fn_8014CBA0(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_80154C74(self);
        fn_8012F5B8(self, 8, 4, 0);
        fn_8012933C(self, 0, 5, 2);
        fn_80134004(self, 0, lbl_80796F08);
        break;
    case 1:
        if (fn_80134114(self, 0, 0x40) == 1) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0x2F, 0, 0);
            fn_80129668(self, 0, 1);
            return;
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014CC98 - scale the offset from the distance to the target and hold it while the frame runs
 * ------------------------------------------------------------------------------------------------- */

void fn_8014CC98(_ENEMY_WORK *self) {
    f32 temp_f1;
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0x36, 2, 0);
        fn_80129668(self, 0, 0xC);
        fn_801353E4(self);
        temp_f1 = fn_80050EF4(&self->v_0x188, &self->v_0x36c);
        if (temp_f1 > lbl_80796EF8) {
            self->v_0x310.z = lbl_80796EA0;
            return;
        }
        if (temp_f1 > lbl_80796F64) {
            self->v_0x310.z = (lbl_80796EA0 * temp_f1) / lbl_80796F4C;
            return;
        }
        self->v_0x310.z = lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796ED8, lbl_80796E1C) == 0) {
            fn_801354F4(self, &self->field_0x1bc);
            fn_80133C50(self, 0x40);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_801280AC(self);
        }
        return;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * fn_8014CDD0 - the turn-and-hold action: hold the offset while the frame check runs, then finish
 * ------------------------------------------------------------------------------------------------- */

void fn_8014CDD0(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0x37, 2, 0);
        fn_80129668(self, 0, 2);
        if ((arg1 & 0xFF) == 1) {
            fn_801353F8(self);
            self->v_0x310.z = lbl_80796E0C;
            self->v_0x320.y = lbl_80796F68;
            return;
        }
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_801280AC(self);
            return;
        }
        if (((arg1 & 0xFF) == 1) && (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796ED8, lbl_80796E1C) == 0)) {
            fn_801355C8(self, &self->field_0x1bc);
            if (self->v_0x310.z < lbl_80796E1C) {
                self->v_0x310.z = lbl_80796E1C;
            }
        }
        break;
    }
}

/* fn_8014C6DC */
void fn_8014C6DC(_ENEMY_WORK *self, u8 arg1) {
    f32 temp_f31;
    f32 temp_f31_2;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r5 + 1);
        fn_80130478(self, 0);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xCD, 6, 0);
        return;
    case 1:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0xCE, 0, 0);
            fn_80129668(self, 0, 7);
            fn_801353F8(self);
            self->v_0x310.z = (f32) (lbl_80796F60 * fn_8012F8E4(self));
            self->v_0x320.y = (f32) (lbl_80796F38 * fn_8012F8E4(self));
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        self->v_0x310.y = fn_80135644(self, &lbl_805A1E50);
        temp_f31 = fn_80130248(self);
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            fn_80135600(self, &self->field_0x1bc);
            if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->v_0x310.z < lbl_80796E1C)) {
                self->sub_state = (u8) (self->sub_state + 1);
                self->v_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:                                     /* switch 2 */
            fn_80135584(self, &self->field_0x1bc);
            break;
        }
        if ((fn_80130008(self, temp_f31) == 1U) && (fn_8012F93C(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            switch ((arg1 & 0xFF)) {                   /* switch 3; irregular */
            case 0:                                 /* switch 3 */
                fn_80130478(self, 0);
                fn_8012F5B8(self, 0xCF, 0, 0);
                fn_80129668(self, 0, 0x16);
                return;
            case 1:                                 /* switch 3 */
                fn_80130478(self, 3);
                fn_8012F5B8(self, 0x1B, 6, 0);
                return;
            case 2:                                 /* switch 3 */
                fn_80130478(self, 0);
                fn_8012F5B8(self, 0xD2, 0, 0);
                fn_80129668(self, 0, 0x16);
                return;
            default:                                /* switch 3 */
                return;
            }
        } else {
            /* Duplicate return node #51. Try simplifying control flow for better match */
            return;
        }
        break;
    case 3:                                         /* switch 1 */
        switch ((arg1 & 0xFF)) {                       /* switch 4; irregular */
        case 0:                                     /* switch 4 */
            if (fn_8012F93C(self) == 1U) {
                fn_80127F48(self);
                return;
            }
            /* Duplicate return node #51. Try simplifying control flow for better match */
            return;
        case 1:                                     /* switch 4 */
            fn_80154CA4(self);
            if (fn_8012F93C(self) == 1U) {
                fn_801280AC(self);
                return;
            }
            /* Duplicate return node #51. Try simplifying control flow for better match */
            return;
        case 2:                                     /* switch 4 */
            if (fn_8012F93C(self) == 1U) {
                self->state = (u8) (self->state + 1);
                fn_8012F5B8(self, 0xCD, 0, 0x46);
                return;
            }
            /* Duplicate return node #51. Try simplifying control flow for better match */
            return;
        default:                                    /* switch 4 */
            return;
        }
        break;
    case 4:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0xCE, 0, 0);
            fn_80129668(self, 0, 7);
            fn_801353F8(self);
            self->v_0x310.z = (f32) (lbl_80796F60 * fn_8012F8E4(self));
            self->v_0x320.y = (f32) (lbl_80796F38 * fn_8012F8E4(self));
            return;
        }
        /* Duplicate return node #51. Try simplifying control flow for better match */
        return;
    case 5:                                         /* switch 1 */
        self->v_0x310.y = fn_80135644(self, &lbl_805A1E50);
        temp_f31_2 = fn_80130248(self);
        temp_r0_2 = self->sub_state;
        switch ((s32) temp_r0_2) {                  /* switch 5; irregular */
        case 0:                                     /* switch 5 */
            fn_80135600(self, &self->field_0x1bc);
            if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->v_0x310.z < lbl_80796E1C)) {
                self->sub_state = (u8) (self->sub_state + 1);
                self->v_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:                                     /* switch 5 */
            fn_80135584(self, &self->field_0x1bc);
            break;
        }
        if ((fn_80130008(self, temp_f31_2) == 1U) && (fn_8012F93C(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xCF, 0, 0);
            fn_80129668(self, 0, 0x16);
            return;
        }
        /* Duplicate return node #51. Try simplifying control flow for better match */
        return;
    case 6:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        /* Duplicate return node #51. Try simplifying control flow for better match */
        return;
    default:                                        /* switch 1 */
        return;
    }
}

/* fn_8014D504 */
void fn_8014D504(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r3;

    fn_8012CF20(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xC8, 6, 0);
        fn_80129668(self, 0, 0x17);
        fn_801353F8(self);
        self->v_0x320.z = fn_801356A8(self, lbl_80796F74, lbl_80796E20, lbl_80796F78);
        return;
    case 1:                                         /* switch 1 */
        fn_80135644(self, &lbl_805A1DD8);
        fn_801303FC(self);
        if (self->field_0x1ac < lbl_80796E24) {
            self->field_0x1ac = (f32) lbl_80796E24;
        }
        self->v_0x310.z = (f32) (self->v_0x320.z * fn_80135644(self, &lbl_805A1E20));
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1bc);
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F58, lbl_80796E1C) == 0) {
            fn_80133C50(self, 0x100);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xDC, 0, 0);
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
            case 1:                                 /* switch 2 */
                if (fn_8012D1A0(self) == 1U) {
                    if (fn_8012D3E0(self, 0) != 0xFF) {
                        fn_8012B380(self, 1, 2);
                        fn_80128A14(self, 8, 0);
                        return;
                    }
                    fn_80128A14(self, 1, 3);
                }
            }
            break;
        }
        break;
    }
}

/* fn_8014D714 */
void fn_8014D714(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_8012CF20(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xD1, 6, 0);
        fn_8012933C(self, 0, 6, 8);
        fn_8012933C(self, 1, 3, 0x10);
        if ((u8) self->field_0x03 == 2) {
            self->field_0xa7e = 0xDE;
        } else {
            self->field_0xa7e = 0xD4;
        }
        fn_801353F8(self);
        fn_801303EC(self, lbl_80796E1C);
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796E3C, lbl_80796E1C) == 0) {
            fn_80133C50(self, 0x80);
        }
        fn_80135644(self, &lbl_805A1E90);
        fn_801303FC(self);
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F7C, lbl_80796E1C) == 1U) {
            if (self->field_0x1ac < lbl_80796E1C) {
                self->field_0x1ac = (f32) lbl_80796E1C;
            }
        } else if (self->field_0x1ac < lbl_80796F34) {
            self->field_0x1ac = (f32) lbl_80796F34;
        }
        self->v_0x310.z = fn_80135644(self, &lbl_805A1F00);
        fn_801354F4(self, &self->field_0x1bc);
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xD4, 0, 0);
            return;
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* fn_8014E160 */
void fn_8014E160(_ENEMY_WORK *self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0x38, 6, 0);
        fn_801353F8(self);
        fn_801303EC(self, lbl_80796E1C);
        fn_80129668(self, 0, 0x1E);
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796E60, lbl_80796E1C) == 0) {
            fn_80133C50(self, 0x100);
        }
        fn_80135644(self, &lbl_805A1D18);
        fn_801303FC(self);
        temp_f1 = self->field_0x1ac;
        if (temp_f1 > lbl_80796E1C) {
            self->field_0x1ac = (f32) lbl_80796E1C;
        } else if (temp_f1 < lbl_80796F08) {
            self->field_0x1ac = (f32) lbl_80796F08;
        }
        self->v_0x310.z = fn_80135644(self, &lbl_805A1D40);
        fn_801354F4(self, &self->field_0x1bc);
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        return;
    }
}

/* fn_8014E3AC */
void fn_8014E3AC(_ENEMY_WORK *self) {
    switch (self->field_0x1e6) {
    case 0x0: fn_8014B630(self); break;
    case 0x1: fn_8014B6CC(self, 0); break;
    case 0x2: fn_8014BB40(self, 0); break;
    case 0x3: fn_8014BDAC(self, 0, 0, 0); break;
    case 0x4: fn_8014BDF8(self, 0); break;
    case 0x5: fn_8014BFE4(self, 0); break;
    case 0x6: fn_8014C16C(self, 0); break;
    case 0x7: fn_8014C16C(self, 1); break;
    case 0x8: fn_8014C464(self, 0); break;
    case 0x9: fn_8014C6DC(self, 0); break;
    case 0xA: fn_8014BB40(self, 1); break;
    case 0xB: fn_8014BB40(self, 2); break;
    case 0xC: fn_8014BDF8(self, 1); break;
    case 0xD: fn_8014CBA0(self); break;
    case 0xE: fn_8014C6DC(self, 1); break;
    case 0xF: fn_8014BB40(self, 3); break;
    case 0x10: fn_8014C16C(self, 2); break;
    case 0x11: fn_8014CC98(self); break;
    case 0x12: fn_8014CDD0(self, 0); break;
    case 0x13: fn_8014C16C(self, 3); break;
    case 0x14: fn_8014B6CC(self, 1); break;
    case 0x15: fn_8014CDD0(self, 1); break;
    case 0x16: fn_8014CEF8(self, 0, 0); break;
    case 0x17: fn_8014CEF8(self, 1, 0); break;
    case 0x18: fn_8014CEF8(self, 0, 1); break;
    case 0x19: fn_8014CEF8(self, 1, 1); break;
    case 0x1A: fn_8014CEF8(self, 0, 2); break;
    case 0x1B: fn_8014CEF8(self, 1, 2); break;
    case 0x1C: fn_8014CEF8(self, 0, 3); break;
    case 0x1D: fn_8014CEF8(self, 1, 3); break;
    case 0x1E: fn_8014C6DC(self, 2); break;
    case 0x1F: fn_8014D504(self); break;
    case 0x20: fn_8014C16C(self, 4); break;
    case 0x21: fn_8014D714(self); break;
    case 0x22: fn_8014C464(self, 1); break;
    case 0x23: fn_8014D8F0(self, 0, 0); break;
    case 0x24: fn_8014DAF4(self, 0, 0); break;
    case 0x25: fn_8014DDEC(self); break;
    case 0x26: fn_8014DAF4(self, 1, 0); break;
    case 0x27: fn_8014DAF4(self, 2, 0); break;
    case 0x28: fn_8014DAF4(self, 3, 0); break;
    case 0x29: fn_8014BDAC(self, 1, 0, 0); break;
    case 0x2A: fn_8014B6CC(self, 2); break;
    case 0x2B: fn_8014BDAC(self, 0, 1, 0); break;
    case 0x2C: fn_8014BDAC(self, 1, 1, 0); break;
    case 0x2D: fn_8014E160(self); break;
    case 0x2E: fn_8014D8F0(self, 1, 0); break;
    case 0x2F: fn_8014D8F0(self, 2, 0); break;
    case 0x30: fn_8014D8F0(self, 3, 0); break;
    case 0x31: fn_8014DAF4(self, 0, 1); break;
    case 0x32: fn_8014DAF4(self, 1, 1); break;
    case 0x33: fn_8014DAF4(self, 2, 1); break;
    case 0x34: fn_8014DAF4(self, 3, 1); break;
    case 0x35: fn_8014BDAC(self, 0, 0, 1); break;
    case 0x36: fn_8014BDAC(self, 1, 0, 1); break;
    case 0x37: fn_8014BDAC(self, 0, 1, 1); break;
    case 0x38: fn_8014BDAC(self, 1, 1, 1); break;
    case 0x39: fn_8014CEF8(self, 2, 3); break;
    case 0x3A: fn_8014E290(self); break;
    case 0x3B: fn_8014DAF4(self, 4, 0); break;
    case 0x3C: fn_8014DAF4(self, 4, 1); break;
    case 0x3D: fn_8014BFE4(self, 1); break;
    case 0x3E: fn_8014D8F0(self, 0, 1); break;
    case 0x3F: fn_8014D8F0(self, 1, 1); break;
    case 0x40: fn_8014D8F0(self, 2, 1); break;
    case 0x41: fn_8014D8F0(self, 3, 1); break;
    }
}

/* fn_8014EA70 */
void fn_8014EA70(_ENEMY_WORK *self) {
    if ((s32) self->field_0x1e6 == 0) {
        fn_8014E774(self);
    }
}

/* fn_8014EA84 */
void fn_8014EA84(_ENEMY_WORK *self) {
    f32 temp_f2;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    fn_80131EC0();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0U;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xCA, 4, 0);
        return;
    case 1:                                         /* switch 1 */
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            if (fn_8012F93C(self) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
            case 1:                                 /* switch 2 */
                if (fn_8012D1A0(self) == 1U) {
                    if ((fn_8012D23C(self, 1, self->field_0x382) == 1U) && (fn_8014E670(self, self->field_0x382) == 1U)) {
                        temp_r0_2 = self->field_0x1e7 - 1;
                        self->field_0x1e7 = temp_r0_2;
                        if ((s32) temp_r0_2 == 0) {
                            if ((u8) self->field_0x03 == 2) {
                                if (fn_8012EC3C(self) == 1U) {
                                    fn_80128A14(self, 9, 4);
                                    return;
                                }
                                fn_80128A14(self, 9, 3);
                                return;
                            }
                            fn_80128A14(self, 9, 2);
                            return;
                        }
                        fn_80130CDC(self, 0x96);
                        temp_f2 = (f32) self->field_0x7a4;
                        fn_8011E6EC(self, (s32) (lbl_80796F9C * temp_f2), 1, lbl_80796E20, temp_f2);
                        fn_80128A14(self, 9, 1);
                        return;
                    }
                    fn_80128A14(self, 2, 6);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

/* fn_8014EC50 */
void fn_8014EC50(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    fn_80131EC0();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0U;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xCB, 4, 0);
        temp_r0 = self->field_0x1e7;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 3:                                     /* switch 2 */
            fn_80129668(self, 0, 0x18);
            return;
        case 2:                                     /* switch 2 */
            fn_80129668(self, 0, 0x19);
            return;
        case 1:                                     /* switch 2 */
            fn_80129668(self, 0, 0x1A);
            return;
        }
        break;
    case 1:                                         /* switch 1 */
        temp_r0_2 = self->sub_state;
        switch ((s32) temp_r0_2) {                  /* switch 3; irregular */
        case 0:                                     /* switch 3 */
            if (fn_8012F93C(self) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
                fn_8012F5B8(self, 0xCA, 4, 0);
            case 1:                                 /* switch 3 */
                if (fn_8012D1A0(self) == 1U) {
                    if ((fn_8012D23C(self, 1, self->field_0x382) == 1U) && (fn_8014E670(self, self->field_0x382) == 1U)) {
                        fn_80128A14(self, 9, 0);
                        return;
                    }
                    fn_80128A14(self, 2, 6);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

/* fn_8014EDC8 */
void fn_8014EDC8(_ENEMY_WORK *self) {
    f32 temp_f31;
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xCD, 6, 0);
        return;
    case 1:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_803B9BA0(self, &self->v_0x188, 0x64);
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0xCE, 0, 0);
            fn_80129668(self, 0, 0x1C);
            fn_801353F8(self);
            self->v_0x310.z = (f32) (lbl_80796F60 * fn_8012F8E4(self));
            self->v_0x320.y = (f32) (lbl_80796F38 * fn_8012F8E4(self));
            fn_802B9574(0);
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        self->v_0x310.y = fn_80135644(self, &lbl_805A1E50);
        temp_f31 = fn_80130248(self);
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            fn_80135600(self, &self->field_0x1bc);
            if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->v_0x310.z < lbl_80796E1C)) {
                self->sub_state = (u8) (self->sub_state + 1);
                self->v_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:                                     /* switch 2 */
            fn_80135584(self, &self->field_0x1bc);
            break;
        }
        if ((fn_80130008(self, temp_f31) == 1U) && (fn_8012F93C(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xCF, 0, 0);
            if (fn_80135748(self, 1) == 0) {
                fn_80129668(self, 0, 0x16);
                return;
            }
        }
        /* Duplicate return node #21. Try simplifying control flow for better match */
        return;
    case 3:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        /* Duplicate return node #21. Try simplifying control flow for better match */
        return;
    default:                                        /* switch 1 */
        return;
    }
}

/* fn_8014F010 */
void fn_8014F010(void) {
    fn_801493A8(0U, 1U, 1);
}

/* fn_8014F020 */
void fn_8014F020(void) {
    fn_801493A8(0U, 3U, 1);
}

/* fn_8014F030 */
void fn_8014F030(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->field_0x1e6;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0:
        fn_8014EA84(self);
        return;
    case 1:
        fn_8014EC50(self);
        return;
    case 2:
        fn_8014EDC8(self);
        return;
    case 3:
        fn_8014F010();
        return;
    case 4:
        fn_8014F020();
        return;
    }
}

/* fn_8014F530 */
void fn_8014F530(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->field_0x1e6;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0:
        fn_801251D8(&lbl_805A2AA0, 0, 0);
        return;
    case 5:
        fn_801251D8(&lbl_805A2B00, 1, 5);
        return;
    case 15:
        fn_801251D8(&lbl_805A2B50, 0, 0xF);
        return;
    case 26:
        fn_801251D8(&lbl_805A2B98, 0, 0x1A);
        return;
    case 28:
        fn_801251D8(&lbl_805A2BC8, 0, 0x1C);
        return;
    default:
        fn_801251D8(&lbl_805A2AA0, 0, 0);
        return;
    }
}

/* fn_8014F5D8 */
void fn_8014F5D8(void) {
    fn_80149AFC();
}

/* fn_8014F5DC */
void fn_8014F5DC(_ENEMY_WORK *self) {
    if ((s32) self->field_0x1e6 == 0) {
        fn_8014F5D8();
    }
}

/* fn_8014F5F0 */
void fn_8014F5F0(_ENEMY_WORK *self) {
    f32 temp_f1;
    f32 temp_f2;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 0xA, 0);
        fn_80134E8C(self);
        self->field_0x20 = 0x12C;
        return;
    case 1:
        temp_f1 = self->v_0x188.y;
        temp_f2 = self->v_0x36c.y - temp_f1;
        if (temp_f2 < lbl_80796FA0) {
            fn_80134F18(self);
        } else if (temp_f2 > lbl_80796F54) {
            self->v_0x188.y = (f32) (temp_f1 + lbl_80796F84);
        }
        temp_f31 = self->v_0x36c.y - self->v_0x188.y;
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if ((temp_r0 <= 0) || ((fn_80133C50(self, 0x180) == 1U) && (temp_f31 >= lbl_80796FA4) && (temp_f31 <= lbl_80796FA8))) {
            fn_80128A14(self, 0xD, 1);
        }
        return;
    default:
        return;
    }
}

/* fn_8014FA34 */
void fn_8014FA34(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0xC8, 0x14, 0);
        fn_80129668(self, 0, 0xB);
        fn_801353F8(self);
        fn_801303EC(self, lbl_80796E1C);
        self->v_0x320.z = fn_801356A8(self, lbl_80796F74, lbl_80796E20, lbl_80796F78);
        return;
    case 1:                                         /* switch 1 */
        fn_80135644(self, &lbl_805A1DD8);
        fn_801303FC(self);
        if (self->field_0x1ac < lbl_80796E24) {
            self->field_0x1ac = (f32) lbl_80796E24;
        }
        self->v_0x310.z = (f32) (self->v_0x320.z * fn_80135644(self, &lbl_805A1E20));
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1bc);
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F58, lbl_80796E1C) == 0) {
            fn_80133C50(self, 0x100);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xDC, 0, 0);
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
            case 1:                                 /* switch 2 */
                if (fn_8012D1A0(self) == 1U) {
                    if (fn_80154638(self, 0) == 1U) {
                        fn_80128A14(self, 0xD, 3);
                        return;
                    }
                    fn_80128A14(self, 1, 3);
                }
            }
            break;
        }
        break;
    }
}

/* fn_8014FE00 */
void fn_8014FE00(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xCB, 4, 0);
        fn_803B9BA0(self, &self->v_0x188, 0x64);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0xCB, 4, 0);
            return;
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0xCB, 4, 0);
            return;
        }
        break;
    case 3:
        if (fn_8012F93C(self) == 1U) {
            fn_80130CDC(self, 0x3E8);
            fn_80128A14(self, 2, 6);
        }
        break;
    }
}

/* fn_8014F138 */
void fn_8014F138(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->field_0x1e6;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0x17:
        fn_801251D0(&lbl_805A2338, 0, 0x17);
        return;
    case 0x18:
        fn_801251D0(&lbl_805A2490, 0, 0x18);
        return;
    case 0x19:
        fn_801251D0(&lbl_805A2490, 0, 0x19);
        return;
    case 0x1A:
        fn_801251D0(&lbl_805A23E0, 0, 0x1A);
        return;
    case 0x1B:
        fn_801251D0(&lbl_805A2450, 0, 0x1B);
        return;
    case 0x1C:
        fn_801251D0(&lbl_805A2360, 0, 0x1C);
        return;
    case 0x1D:
        fn_801251D0(&lbl_805A2388, 0, 0x1D);
        return;
    case 0x1E:
        fn_801251D0(&lbl_805A2360, 0, 0x1E);
        return;
    case 0x23:
        fn_801251D0(&lbl_805A2518, 1, 0x23);
        return;
    case 0x7A:
        fn_801251D0(&lbl_805A2588, 0, 0x7A);
        return;
    case 0x7B:
        fn_801251D0(&lbl_805A25B0, 0, 0x7B);
        return;
    case 0x7C:
        fn_801251D0(&lbl_805A2620, 0, 0x7C);
        return;
    case 0x8D:
        fn_801251D0(&lbl_805A2718, 0, 0x8D);
        return;
    case 0x7E:
        fn_801251D0(&lbl_805A2678, 0, 0x7E);
        return;
    case 0x7F:
        fn_801251D0(&lbl_805A26E0, 0, 0x7F);
        return;
    case 0x8E:
        fn_801251D0(&lbl_805A2718, 0, 0x8E);
        return;
    case 0x78:
        fn_801251D0(&lbl_805A2808, 0, 0x78);
        return;
    case 0x84:
        fn_801251D0(&lbl_805A27A0, 1, 0x84);
        return;
    case 0x9F:
        fn_801251D0(&lbl_805A25F0, 0, 0x9F);
        return;
    case 0xA0:
        fn_801251D0(&lbl_805A25F0, 0, 0xA0);
        return;
    case 0xA8:
        fn_801251D0(&lbl_805A2830, 0, 0xA8);
        return;
    case 0xA9:
        fn_801251D0(&lbl_805A2518, 1, 0xA9);
        return;
    case 0xB6:
        fn_801251D0(&lbl_805A2858, 0, 0xB6);
        return;
    case 0xB7:
        fn_801251D0(&lbl_805A2880, 0, 0xB7);
        return;
    case 0xB8:
        fn_801251D0(&lbl_805A28F0, 1, 0xB8);
        return;
    case 0xB9:
        fn_801251D0(&lbl_805A2978, 0, 0xB9);
        return;
    case 0xBA:
        fn_801251D0(&lbl_805A29A0, 0, 0xBA);
        return;
    case 0xBB:
        fn_801251D0(&lbl_805A2950, 0, 0xBB);
        return;
    case 0xBC:
        fn_801251D0(&lbl_805A2950, 0, 0xBC);
        return;
    case 0xBF:
        fn_801251D0(&lbl_805A29E0, 0, 0xBF);
        return;
    case 0xC1:
        fn_801251D0(&lbl_805A2718, 0, 0xC1);
        return;
    case 0xCA:
        fn_801251D0(&lbl_805A2A58, 0, 0xCA);
        return;
    case 0xF0:
        fn_801251D0(&lbl_805A2830, 0, 0xF0);
        return;
    default:
        fn_80127F48(self);
        return;
    }
}

/* fn_8014F078 */
void fn_8014F078(_ENEMY_WORK *self) {
    f32 spC;
    VEC3 sp8;

    fn_80043EA8(&sp8);
    if ((u32) (em_get_mot_no__FP11_ENEMY_WORK(self) - 0x70) <= 1U) {
        if (((s32) self->sub_state == 0) && (em_magma_check__FP11_ENEMY_WORK(self) == 1U) && (self->v_0x188.y < self->field_0x214)) {
            self->sub_state = (u8) (self->sub_state + 1);
            fn_80041E40(&sp8, &self->v_0x188);
            spC = lbl_80796F00 + self->field_0x214;
            eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl(0x88, &sp8, &self->field_0x1bc, self->field_0x1e1, lbl_80796E20);
        }
    } else {
        self->sub_state = 0U;
    }
}

/* fn_8014FF10 */
void fn_8014FF10(_ENEMY_WORK *self) {
    VEC3 sp8;
    u8 temp_r3;

    fn_80043EA8(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x13, 6, 0);
        return;
    case 1:
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E4C, lbl_80796E1C) == 1U) || (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E50, lbl_80796E1C) == 1U)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x33, 0x1A, &sp8, lbl_80796E20);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E5C, lbl_80796E1C) == 1U) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
            fn_801049D0(self, 0x1A, 0x2F, 0, &sp8, lbl_80796E64);
        }
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796E68, lbl_80796E6C) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x34, 0x1A, &sp8, lbl_80796E20);
        }
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E78, lbl_80796E1C) == 1U) || (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E7C, lbl_80796E1C) == 1U) || (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E80, lbl_80796E1C) == 1U)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x33, 0x1A, &sp8, lbl_80796E84);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E50, lbl_80796E1C) == 1U) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
            fn_801049D0(self, 0x1A, 0x2F, 0, &sp8, lbl_80796E64);
        }
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796E70, lbl_80796E74) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x34, 0x1A, &sp8, lbl_80796E20);
        }
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796E88, lbl_80796E8C) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x34, 0x1A, &sp8, lbl_80796E98);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796E9C, lbl_80796E1C) == 1U) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x33, 0x1A, &sp8, lbl_80796E2C);
        }
        if ((em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796E90, lbl_80796E94) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            fn_80304508(self, 0x34, 0x1A, &sp8, lbl_80796E98);
        }
        return;
    }
}

/* fn_8014CEF8 */
void fn_8014CEF8(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    VEC3 sp1C;
    VEC3 sp10;
    u32 spC;
    s32 sp8;
    s32 temp_r0;
    s32 temp_r0_2;
    s32 temp_r0_4;
    s32 temp_r3_2;
    s32 temp_r3_3;
    u8 temp_r0_3;
    u8 temp_r0_5;
    u8 temp_r3;

    fn_80043EA8(&sp1C);
    fn_8012CF20(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_80154C74(self);
        fn_8012F5B8(self, 0x45, 6, 0);
        self->sub_state = 0U;
        fn_801353F8(self);
        self->v_0x310.y = (f32) lbl_80796F6C;
        self->v_0x310.z = (f32) lbl_80796E60;
        return;
    case 1:                                         /* switch 1 */
        fn_80133C3C(self);
        fn_80136D4C(self, lbl_80796F00);
        switch ((s32) arg2) {                       /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            spC = 0x2800;
            break;
        case 1:                                     /* switch 2 */
            spC = 0x2000;
            break;
        case 2:                                     /* switch 2 */
            spC = 0x1800;
            break;
        default:                                    /* switch 2 */
            fn_80050CA0(&sp10, &self->v_0x36c, &self->v_0x188);
            fn_80041E40(&sp1C, &sp10);
            calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl(&sp1C, &spC, &sp8);
            if (spC > 0x3000U) {
                spC = 0x3000;
            }
            break;
        }
        self->field_0x1bc = (s32) fn_80133DB0((u16) spC, (u16) self->field_0x1bc, 0x180);
        fn_80133C50(self, 0x280);
        fn_80135600(self, &self->field_0x1bc, fn_80130248(self) - lbl_80796F4C);
        if (self->v_0x310.z > lbl_80796F28) {
            self->v_0x310.z = (f32) lbl_80796F28;
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0x3E, 6, 0);
            self->field_0x20 = 0;
            fn_8012933C(self, 0, 0xE, 3);
            self->v_0x320.y = (f32) lbl_80796F04;
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        fn_80133C3C(self);
        self->field_0x20 = (s32) (self->field_0x20 + 1);
        if (fn_80135600(self, &self->field_0x1bc, fn_80130248(self) - lbl_80796F4C) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x3F, 6, 0);
            fn_801303EC(self, lbl_80796F34);
        } else if (((s32) self->field_0x20 > 0x96) && (fn_8012D1A0(self) == 1U)) {
            fn_80127FE4(self);
        }
        if (self->v_0x310.z > lbl_80796E34) {
            self->v_0x310.z = (f32) lbl_80796E34;
            return;
        }
        break;
    case 3:                                         /* switch 1 */
        fn_801303EC(self, lbl_80796F34);
        fn_801354F4(self, &self->field_0x1bc);
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0xD0, 0, 0);
            self->field_0x20 = 0;
            return;
        }
        break;
    case 4:                                         /* switch 1 */
        fn_801303EC(self, lbl_80796F34);
        fn_801354F4(self, &self->field_0x1bc);
        temp_r0 = self->field_0x20 + 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 > 6) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    fn_8012F5B8(self, 0xD2, 6, 0);
                    self->v_0x320.y = (f32) lbl_80796F70;
                    self->field_0x20 = 0;
                    return;
                }
                fn_8012F5B8(self, 0x35, 0xA, 0);
                return;
            }
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xDD, 0, 0);
            fn_801303EC(self, lbl_80796E1C);
            return;
        }
        break;
    case 5:                                         /* switch 1 */
        if ((arg1 & 0xFF) != 1) {
            if ((arg1 & 0xFF) != 2) {
                temp_r3_2 = self->field_0x20;
                if (temp_r3_2 < 0x14) {
                    temp_r0_2 = temp_r3_2 + 1;
                    self->field_0x20 = temp_r0_2;
                    fn_801303EC(self, (lbl_80796F3C * (f32) (0x14 - temp_r0_2)) / lbl_80796E0C);
                }
                if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    fn_80129724(self, 0);
                }
                fn_801355C8(self, &self->field_0x1bc);
                if (self->v_0x310.z < lbl_80796E1C) {
                    self->v_0x310.z = (f32) lbl_80796E1C;
                }
                if (fn_8012F93C(self) == 1U) {
                    fn_801280AC(self);
                    return;
                }
            } else {
                temp_r0_3 = self->sub_state;
                switch ((s32) temp_r0_3) {          /* switch 4; irregular */
                case 0:                             /* switch 4 */
                    temp_r3_3 = self->field_0x20;
                    if (temp_r3_3 < 0xA) {
                        temp_r0_4 = temp_r3_3 + 1;
                        self->field_0x20 = temp_r0_4;
                        fn_801303EC(self, (lbl_80796F34 * (f32) (5 - temp_r0_4)) / lbl_80796F00);
                    }
                    if (fn_8012F93C(self) == 1U) {
                        self->sub_state = (u8) (self->sub_state + 1);
                        fn_80129724(self, 0);
                        fn_80130478(self, 0);
                        fn_8012F5B8(self, 0x3C, 0, 0);
                        return;
                    }
                    break;
                case 1:                             /* switch 4 */
                    if (fn_8012F93C(self) == 1U) {
                        fn_80127F48(self);
                    }
                    break;
                }
            }
        } else {
            temp_r0_5 = self->sub_state;
            switch ((s32) temp_r0_5) {              /* switch 3; irregular */
            case 0:                                 /* switch 3 */
                if (fn_8012F93C(self) == 1U) {
                    self->sub_state = (u8) (self->sub_state + 1);
                    fn_8012F5B8(self, 0xDA, 0, 0);
                    return;
                }
                break;
            case 1:                                 /* switch 3 */
                if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    fn_80129724(self, 0);
                }
                if (fn_8012F93C(self) == 1U) {
                    fn_80127F48(self);
                    return;
                }
                break;
            }
        }
        break;
    }
}

/* fn_8014E670 */
u32 fn_8014E670(_ENEMY_WORK *self, u8 arg1) {
    VEC3 sp2C;
    VEC3 sp20;
    VEC3 sp14;
    VEC3 sp8;
    s32 temp_r31;
    u8 *temp_r31_2;

    fn_80043EA8(&sp2C);
    fn_80043EA8(&sp20);
    temp_r31 = get_move_work_adrs__FUc(2);
    if ((arg1 & 0xFF) < (s32) get_move_work_max__FUc(2)) {
        temp_r31_2 = (u8 *) (temp_r31 + (arg1 * 0xB20));
        if ((s32) *temp_r31_2 != 0) {
            setVector3__FPQ34nw4r4math4VEC3fff(&sp2C, lbl_80796E1C, lbl_80796E1C, lbl_80796E20);
            rotVecY__FPQ34nw4r4math4VEC3Ul(&sp2C, self->field_0x1c0);
            fn_80051EE0(&sp8, &sp2C, lbl_80796F64 * get_em_chg_scale__FP11_ENEMY_WORK(self));
            fn_80051378(&sp14, &self->v_0x188, &sp8);
            fn_80041E40(&sp20, &sp14);
            if (fn_80050EAC(temp_r31_2 + 0x3C, &sp20) <= lbl_80796F94) {
                return 1U;
            }
        }
    }
    return 0U;
}

/* fn_8014FC24 */
void fn_8014FC24(_ENEMY_WORK *self) {
    VEC3 sp3C;
    VEC3 sp30;
    VEC3 sp24;
    VEC3 sp18;
    VEC3 spC;
    u16 sp8;
    f32 temp_f31;
    u8 temp_r3;

    fn_80043EA8(&sp3C);
    fn_80043EA8(&sp30);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0;
        fn_80130478(self, 0);
        fn_80154B04(self, &sp3C, &sp8, lbl_80796F64);
        fn_80050CA0(&sp24, &sp3C, &self->v_0x188);
        fn_80041E40(&sp30, &sp24);
        fn_80128BF8(self, &sp3C);
        fn_80134964(self, &lbl_8056F960, 2, 1, sp8);
        fn_801353E4(self);
        temp_f31 = fn_8012F8E4(self);
        fn_80051EE0(&spC, &sp30, lbl_80796F98);
        fn_80051EE0(&sp18, &spC, temp_f31);
        fn_80041E40(&self->v_0x310, &sp18);
        self->v_0x310.y = (f32) lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796EA0, lbl_80796E60) == 1U) {
            fn_80135418(self);
            fn_80134B0C(self, &lbl_8056F960);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0xCA, 4, 0);
            return;
        }
        return;
    case 2:
        if (fn_8012D1A0(self) == 1U) {
            if (fn_80154638(self, 1) == 1U) {
                fn_80128A14(self, 0xD, 4);
                return;
            }
            fn_80128A14(self, 2, 6);
            /* Duplicate return node #13. Try simplifying control flow for better match */
            return;
        }
        /* Duplicate return node #13. Try simplifying control flow for better match */
        return;
    default:
        return;
    }
}

/* fn_8014E290 */
void fn_8014E290(_ENEMY_WORK *self) {
    ShellParams sp8;
    u8 temp_r3;

    fn_80149788(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xDB, 4, 0);
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F8C, lbl_80796E1C) == 1U) {
            fn_80103960(self, 0x16);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F90, lbl_80796E1C) == 1U) {
            fn_801545B8(&sp8, 0, 0x11C7U, 0);
            fn_801039B0(self, 0x15, sp8.field_0x12, sp8.field_0x14);
            shell_set_func_ptr->field_0x3c(self, &sp8, 3, shell_set_func_ptr);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

/* fn_8014BB40 */
void fn_8014BB40(_ENEMY_WORK *self, u8 arg1) {
    ShellParams sp8;
    u16 var_r31;
    u8 temp_r3;

    fn_80149788(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        self->field_0x20 = 0;
        if (arg1 == 3) {
            fn_8012F5B8(self, 0x33, 6, 0);
            return;
        }
        fn_80154C74(self);
        fn_8012F5B8(self, 0x33, 6, 0);
        return;
    case 1:
        if (arg1 == 3) {
            fn_8012F8EC(self);
            getKeyData__FPff(&lbl_805A2078);
            fn_8012F8C8(self);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F40, lbl_80796E1C) == 1U) {
            fn_80103960(self, 3);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F44, lbl_80796E1C) == 0) {
            if ((s32) (self->field_0x20 & 7) == 0) {
                fn_80103960(self, 1);
            }
            self->field_0x20 = (s32) (self->field_0x20 + 1);
        }
        if (arg1 != 3) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    var_r31 = 0;
                } else {
                    var_r31 = 0x71C;
                }
            } else {
                var_r31 = 0xF8E4;
            }
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
                fn_801545B8(&sp8, 0, var_r31, 0);
                shell_set_func_ptr->field_0x3c(self, &sp8, 0, shell_set_func_ptr);
                fn_801039B0(self, 0, sp8.field_0x12, sp8.field_0x14);
                fn_801039B0(self, 2, sp8.field_0x12, sp8.field_0x14);
            }
        } else if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
            fn_801545B8(&sp8, 0, 0U, 0);
            shell_set_func_ptr->field_0x3c(self, &sp8, 1, shell_set_func_ptr);
            fn_801039B0(self, 4, 0U, 0U);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

/* fn_8014C464 */
void fn_8014C464(_ENEMY_WORK *self, u8 arg1) {
    ShellParams sp8;
    s32 var_r31;
    u8 temp_r3;

    fn_80149788(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xD0, 4, 0);
        return;
    case 1:
        if (arg1 == 1) {
            fn_8012F8EC(self);
            getKeyData__FPff(&lbl_805A20A0);
            fn_8012F8C8(self);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F40, lbl_80796E1C) == 1U) {
            fn_80103960(self, 3);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F44, lbl_80796E1C) == 0) {
            if ((s32) (self->field_0x20 & 7) == 0) {
                fn_80103960(self, 1);
            }
            self->field_0x20 = (s32) (self->field_0x20 + 1);
        }
        var_r31 = 0;
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
            fn_801545B8(&sp8, 0, 0x38EU, 0);
            var_r31 = 1;
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F28, lbl_80796E1C) == 1U) {
            fn_801545B8(&sp8, 0, 0x38EU, 0xF334);
            var_r31 = 1;
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F5C, lbl_80796E1C) == 1U) {
            fn_801545B8(&sp8, 0, 0x321U, 0x105B);
            var_r31 = 2;
        }
        if (var_r31 != 0) {
            if ((arg1 & 0xFF) == 0) {
                shell_set_func_ptr->field_0x3c(self, &sp8, 0, shell_set_func_ptr);
                fn_801039B0(self, 0, sp8.field_0x12, sp8.field_0x14);
                if ((u32) var_r31 == 2U) {
                    fn_801039B0(self, 2, sp8.field_0x12, sp8.field_0x14);
                }
            } else {
                shell_set_func_ptr->field_0x3c(self, &sp8, 1, shell_set_func_ptr);
                fn_801039B0(self, 4, sp8.field_0x12, sp8.field_0x14);
            }
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

/* fn_8014D8F0 */
void fn_8014D8F0(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    ShellParams sp8;
    u16 var_r5;
    u8 temp_r3;

    fn_80149788(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        fn_80130478(self, 3);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xCD, 6, 0);
        fn_801353E4(self);
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EA8, lbl_80796E1C) == 0) {
            if ((u8) self->field_0x7c8 <= 0x28U) {
                fn_80133C50(self, 0x80);
            } else {
                fn_80133C50(self, 0x100);
            }
        }
        if ((s32) arg2 == 0) {
            self->v_0x310.z = fn_80135644(self, &lbl_805A1F98);
            fn_801354F4(self, &self->field_0x1bc);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796F80, lbl_80796E1C) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    var_r5 = 0x127D;
                } else {
                    var_r5 = 0x16C1;
                }
            } else {
                var_r5 = 0xE39;
            }
            fn_801545B8(&sp8, 1, var_r5, 0);
            if (arg1 == 3) {
                shell_set_func_ptr->field_0x3c(self, &sp8, 1, shell_set_func_ptr);
                fn_801039B0(self, 4, sp8.field_0x12, sp8.field_0x14);
            } else {
                shell_set_func_ptr->field_0x3c(self, &sp8, 0, shell_set_func_ptr);
                fn_801039B0(self, 0x17, sp8.field_0x12, sp8.field_0x14);
                fn_801039B0(self, 2, sp8.field_0x12, sp8.field_0x14);
            }
        }
        if (fn_8012F93C(self) == 1U) {
            fn_801280AC(self);
        }
        return;
    }
}

/* fn_8014DAF4 */
void fn_8014DAF4(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    ShellParams sp8;
    u16 var_r5;
    u8 temp_r0;
    u8 temp_r3;

    fn_80149788(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xD3, 6, 0);
        if ((u32) (arg1 - 2) > 1U) {
            if ((arg1 & 0xFF) != 4) {
                return;
            }
            fn_801353E4(self);
            self->v_0x310.z = (f32) lbl_80796F0C;
            self->v_0x310.y = (f32) lbl_80796F6C;
            return;
        }
        fn_801353E4(self);
        self->v_0x310.z = (f32) lbl_80796F84;
        return;
    case 1:                                         /* switch 1 */
        fn_80133C50(self, 0x40);
        if ((u32) (arg1 - 2) <= 2U) {
            fn_801354F4(self, &self->field_0x1bc);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_8012F5B8(self, 0xD4, 6, 0);
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        if ((u32) (arg1 - 2) > 2U) {
            if ((arg1 & 0xFF) == 1) {
                fn_80133C50(self, 0x40);
            }
        } else {
            fn_801354F4(self, &self->field_0x1bc);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 0, lbl_80796EA8, lbl_80796E1C) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                var_r5 = 0x238E;
            } else {
                temp_r0 = self->sub_state;
                switch ((s32) temp_r0) {            /* switch 2; irregular */
                case 0:                             /* switch 2 */
                    var_r5 = 0x238E;
                    break;
                default:                            /* switch 2 */
                    var_r5 = 0x1E94;
                    break;
                case 2:                             /* switch 2 */
                    var_r5 = 0x199A;
                    break;
                }
            }
            fn_801545B8(&sp8, 1, var_r5, 0);
            if ((s32) arg2 == 0) {
                shell_set_func_ptr->field_0x3c(self, &sp8, 0, shell_set_func_ptr);
                fn_801039B0(self, 0x17, sp8.field_0x12, sp8.field_0x14);
                fn_801039B0(self, 2, sp8.field_0x12, sp8.field_0x14);
            } else {
                shell_set_func_ptr->field_0x3c(self, &sp8, 1, shell_set_func_ptr);
                fn_801039B0(self, 4, sp8.field_0x12, sp8.field_0x14);
            }
            self->sub_state = (u8) (self->sub_state + 1);
        }
        if ((((arg1 != 1) && (arg1 != 3) && (arg1 != 4)) || ((u8) self->sub_state >= 3U)) && (fn_8012F93C(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            fn_8012F5B8(self, 0xD5, 6, 0);
            return;
        }
        break;
    case 3:                                         /* switch 1 */
        if (fn_8012F93C(self) == 1U) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* fn_8014DDEC */
void fn_8014DDEC(_ENEMY_WORK *self) {
    VEC3 sp14;
    VEC3 sp8;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f31_3;
    s32 temp_r30;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r4 + 1);
        self->sub_state = 0;
        fn_80130248(self);
        fn_801305C4(self);
        fn_80154C74(self);
        fn_8012F5B8(self, 0xDB, 6, 0);
        fn_80129668(self, 0, 0x1F);
        fn_80041E40(&self->v_0x1b0, &self->v_0x188);
        self->field_0x32c = (u16) self->field_0x1c0;
        fn_801353F8(self);
        self->field_0x20 = (s32) (s16) (lbl_80796EA8 / fn_8012F8E4(self));
        return;
    case 1:
        fn_8012CF20(self);
        fn_80128BF8(self, NULL);
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 3e-45f, lbl_80796E34, lbl_80796E1C) == 1U) {
            fn_80133C50(self, 0x300);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796ED8, lbl_80796F88) == 1U) {
            if ((s32) self->field_0x20 > 0) {
                self->v_0x310.x = (f32) lbl_80796E1C;
                temp_f31 = get_em_chg_scale__FP11_ENEMY_WORK(self);
                temp_f1 = ((self->v_0x36c.y - self->v_0x188.y) + ((fn_80130248(self) - lbl_80796F88) * temp_f31)) / (f32) self->field_0x20;
                self->v_0x310.y = temp_f1;
                if (temp_f1 > lbl_80796E1C) {
                    self->v_0x310.y = (f32) lbl_80796E1C;
                }
                if ((u16) ((calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(&self->v_0x188, &self->v_0x36c) - self->field_0x1c0) + 0x4000) < -0x8000U) {
                    temp_f31_2 = lbl_80796F88 * get_em_chg_scale__FP11_ENEMY_WORK(self);
                    temp_f1_2 = (fn_80050F80(&self->v_0x36c, &self->v_0x188) - temp_f31_2) / (f32) self->field_0x20;
                    self->v_0x310.z = temp_f1_2;
                    if (temp_f1_2 == lbl_80796EFC) {
                        self->v_0x310.z = (f32) lbl_80796EFC;
                    }
                } else {
                    self->v_0x310.z = (f32) lbl_80796EFC;
                }
                fn_80130248(self);
                fn_80135584(self, &self->field_0x1bc);
            }
            self->field_0x20 = (s32) (self->field_0x20 - 1);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796F7C, lbl_80796E1C) == 1U) {
            self->state = (u8) (self->state + 1);
            temp_f31_3 = fn_8012F8E4(self);
            self->field_0x20 = (s32) (s16) ((lbl_80796F4C - fn_8012F8EC(self)) / temp_f31_3);
            return;
        }
        return;
    case 2:
        self->field_0x1c0 = (s32) fn_80133DB0(self->field_0x32c, (u16) self->field_0x1c0, 0x300);
        fn_801353E4(self);
        temp_r30 = self->field_0x20;
        if (temp_r30 > 0) {
            fn_80050CA0(&sp8, &self->v_0x1b0, &self->v_0x188);
            fn_800AD9C0(&sp14, &sp8, (f32) temp_r30);
            fn_80041E40(&self->v_0x310, &sp14);
            fn_80135418(self);
        }
        self->field_0x20 = (s32) (self->field_0x20 - 1);
        if (fn_8012F93C(self) == 1U) {
            fn_80127FE4(self);
        }
        return;
    default:
        return;
    }
}

/* fn_8014E774 */
void fn_8014E774(_ENEMY_WORK *self) {
    VEC3 sp48;
    VEC3 sp3C;
    VEC3 sp30;
    VEC3 sp24;
    VEC3 sp18;
    VEC3 spC;
    u16 sp8;
    f32 temp_f2;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r3;

    fn_80043EA8(&sp48);
    fn_80043EA8(&sp3C);
    fn_80043EA8(&sp30);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->sub_state = 0;
        fn_80130478(self, 0);
        fn_80154B04(self, &sp48, &sp8, lbl_80796F64);
        fn_80050CA0(&sp24, &sp48, &self->v_0x188);
        fn_80041E40(&sp3C, &sp24);
        fn_80128BF8(self, &sp48);
        fn_80134964(self, &lbl_8056F960, 2, 1, sp8);
        fn_801353E4(self);
        temp_f31 = fn_8012F8E4(self);
        fn_80051EE0(&spC, &sp3C, lbl_80796F98);
        fn_80051EE0(&sp18, &spC, temp_f31);
        fn_80041E40(&self->v_0x310, &sp18);
        self->v_0x310.y = (f32) lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 2, lbl_80796EA0, lbl_80796E60) == 1U) {
            fn_80135418(self);
            fn_80134B0C(self, &lbl_8056F960);
        }
        if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796E60, lbl_80796E1C) == 1U) {
            fn_80131EC0(self);
        }
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->field_0x20 = 0x5A;
            fn_8012F5B8(self, 0xCA, 4, 0);
            return;
        }
        return;
    case 2:
        fn_80131EC0(self);
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 < 0) {
            self->field_0x20 = 0;
        }
        if (fn_8012D1A0(self) == 1U) {
            if (fn_8014E670(self, self->field_0x382) == 1U) {
                if (fn_8012D23C(self, 1, self->field_0x382) == 1U) {
                    self->field_0x1e7 = 3;
                    if (fn_8012EC3C(self) == 1U) {
                        fn_80130CDC(self, 0x12C);
                    } else {
                        fn_80130CDC(self, 0x96);
                    }
                    temp_f2 = (f32) self->field_0x7a4;
                    fn_8011E6EC(self, (s32) (lbl_80796F9C * temp_f2), 1, lbl_80796E20, temp_f2);
                    fn_80128A14(self, 9, 1);
                    return;
                }
                if (((s32) self->field_0x20 <= 0) || (fn_8012D23C(self, 0, self->field_0x382) == 0)) {
                    fn_80128A14(self, 2, 6);
                    return;
                }
                return;
            }
            fn_80128A14(self, 2, 6);
            return;
        }
        return;
    default:
        return;
    }
}

/* fn_8014F71C */
void fn_8014F71C(_ENEMY_WORK *self) {
    VEC3 sp1C;
    VEC3 sp10;
    u32 spC;
    s32 sp8;
    u32 var_r0;
    u32 var_r30;
    u8 temp_r0;
    u8 temp_r3;

    fn_80043EA8(&sp1C);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state = (u8) (temp_r3 + 1);
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x3D, 6, 0);
        fn_801353F8(self);
        self->v_0x310.z = (f32) lbl_80796E38;
        self->v_0x320.y = (f32) lbl_80796ECC;
        return;
    case 1:                                         /* switch 1 */
        fn_80133C3C(self);
        fn_80130248(self);
        fn_80135600(self, &self->field_0x1bc);
        fn_80050CA0(&sp10, &self->v_0x36c, &self->v_0x188);
        fn_80041E40(&sp1C, &sp10);
        calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl(&sp1C, &spC, &sp8);
        var_r0 = spC + 0x400;
        spC = var_r0;
        if (var_r0 > 0x3000U) {
            var_r0 = 0x3000;
            spC = 0x3000;
        }
        self->field_0x1bc = (s32) fn_80133DB0((u16) var_r0, (u16) self->field_0x1bc, 0x100);
        self->field_0x1c0 = (s32) fn_80133DB0((u16) sp8, (u16) self->field_0x1c0, 0x40);
        if (fn_8012F93C(self) == 1U) {
            self->state = (u8) (self->state + 1);
            fn_801353E4(self);
            self->v_0x310.z = (f32) lbl_80796E3C;
            self->v_0x320.y = (f32) lbl_80796ECC;
            self->v_0x310.y = (f32) lbl_80796F6C;
            fn_8012F5B8(self, 0x3E, 6, 0);
            fn_80134004(self, 0x12, lbl_80796FAC);
            return;
        }
        return;
    case 2:                                         /* switch 1 */
        fn_80133C3C(self);
        if (fn_80134114(self, 0, 0x80U) == 1) {
            self->state = (u8) (self->state + 1);
            self->sub_state = 0U;
            fn_8012F5B8(self, 0x2D, 6, 0);
            self->v_0x310.y = (f32) lbl_80796FB0;
            self->v_0x320.y = (f32) lbl_80796FB4;
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1bc);
            return;
        }
        fn_80130248(self);
        fn_80135600(self, &self->field_0x1bc);
        if (self->v_0x310.z > lbl_80796F7C) {
            self->v_0x310.z = (f32) lbl_80796F7C;
            return;
        }
        break;
    case 3:                                         /* switch 1 */
        temp_r0 = self->sub_state;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            fn_80130248(self);
            var_r30 = fn_80135600(self, &self->field_0x1bc);
            if (em_frame_check__FP11_ENEMY_WORKUsff(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->sub_state = (u8) (self->sub_state + 1);
                self->v_0x320.y = (f32) lbl_80796F14;
            }
            break;
        case 1:                                     /* switch 2 */
            fn_80130248(self);
            var_r30 = fn_80135600(self, &self->field_0x1bc);
            if (self->v_0x310.z < lbl_80796E1C) {
                self->v_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        }
        if (fn_8012F93C(self) == 1U) {
            if (var_r30 == 1U) {
                fn_80130478(self, 3);
                fn_80128A14(self, 0xD, 2);
                return;
            }
            fn_80127FE4(self);
        }
        break;
    }
}
