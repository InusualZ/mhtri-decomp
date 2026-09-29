/*
 * Pl/pl_act_step.cpp - the player's per-act state machine cluster.
 *
 * Proposal `8024F200_fn_8024F200`, registered once at its final home.  `.text`
 * 0x8024F200-0x80258FCC (40396 B, 92 functions): every function takes the player work
 * (`_PLW*`, passed straight to `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no` and
 * to the `Pl/pl_act.cpp` motion helpers) plus the per-part index, reads the act step byte
 * `_PLW+0x005`, and drives one step of that act.  The family is exactly the one
 * `Pl/fn_80230FBC.cpp`'s kind dispatcher calls, so the whole range is worked as one unit: the
 * discovery pass cut it on its `--max-bytes` cap, and nothing inside the range names a file (the
 * `.data` pool between `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at 0x805CDFC8 carries
 * no `__FILE__` string for the whole Pl band), so the seam cannot be shown from the range itself.
 *
 * Home (evidence order): 1. no `__FILE__` string anywhere in the band (see above); 2. no
 * runtime-dump name - `dumpmap.py lookup 0x8024F200` answers only the `zz_024f200_` placeholder;
 * 3. class 3 - the first argument is the `_PLW` every `Pl_*` predicate in this lib takes, and the
 * callees are `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no`, so the module is
 * `Pl`; 4. the stem is the map's `fn_8024F200`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8024F200` -> `zz_024f200_`, and with
 * `grep -n "^fn_8024\|^fn_8025" config/RMHE08/symbols.txt` - every one of the 92 entries is a bare
 * `fn_` placeholder, and every non-mangled callee they share is one too).
 *
 * Language: C++ - the map's undefined set carries real manglings (`Pl_Skill_ck__FP4_PLWUs`,
 * `Pl_cat_skill_ck__FP4_PLWUs`, `Pl_frame_check__FP4_PLWUlff`, `GetGroundHit2__FPQ34nw4r4math4VEC3UlUcPUc`)
 * and the band's objects carry extab/extabindex (the Pl lib's `-Cpp_exceptions on`).
 *
 * Flags: `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw_version Wii/1.0),
 * the set the sibling Pl units measure with.  The per-part argument is `s32`, not `u8`: retail
 * compares it with `cmpwi r4,0` and never masks it, which a `u8` declaration cannot produce.
 *
 * Residual (5 of the range's 92 functions are written; the other 87 have no body yet).  Measured
 * with `recompile.py Pl/pl_act_step.cpp --measure <symbol>`:
 *   * `fn_8024F200` 99.91 - `.text` (260 insns), `extab` and `extabindex` are byte-identical; the
 *     only gap is a *name*: the target's one `R_PPC_ADDR32` in extabindex points at dtk's
 *     `@etb_80011EFC` where MWCC writes its own anonymous `@283` local (the same class of residual
 *     `Pl/fn_80230FBC.cpp` records for its jump tables).  Nothing in the source reaches it.
 *   * `fn_8024F8A8` 98.59 - one extra `clrlwi r0,r0,24` on the case-0 store.  The switch needs a
 *     `u8` local for retail's 3-instruction range test (`addi r0,r3,-1; cmplwi r0,1; ble`); the
 *     `u8` local's own conversion is what the `clrlwi` is, and both alternatives lose the range test
 *     instead (retail's 2-instruction store with the field written `++` in place of the local, or a
 *     `(u8)` cast on the switch operand, both measure 95.35).
 *   * `fn_8024F9C4` 97.70 - 8 B short: MWCC removes the loop's counter (`li r4,0` / `addi r4,r4,1`
 *     are absent, and the address becomes `lhz r0,1436(r3)` off retail's own base) where retail
 *     keeps it.  The shape that keeps the counter (an explicit `u16*` plus `i`, `i++, word++`)
 *     changes the base to `addi r3,r31,1436` and measures 97.00, so the first shape is kept.
 *   * `fn_8024FB20` 100.00 and `fn_80257E70` 100.00.
 *
 * Load-bearing shapes worth copying into the next functions of this range:
 *   * the per-part argument is `s32`, never `u8` - a `u8` declaration makes MWCC mask it
 *     (`clrlwi r...,r4,24`) before every compare, which retail does not do anywhere in the range;
 *   * `++` on a `u8` field stores raw (`addi`/`stb`); `+= 1` and `= x + 1` both add a `clrlwi`;
 *   * a `switch` whose case bodies are shared (`case 1: case 2:`) has its bodies emitted in
 *     *source* order, and a 3-value switch whose 0-arm is written first emits the bodies as
 *     2, 3, 0 - write the cases in the order the target's bodies sit in `.text`.
 */

#include "types.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act_step.h"
#include "Pl/fn_802693C4.h"
#include "unsplit/Pl.h"

/* 0x802DE578 / 0x802F39DC - the two helpers this unit calls whose address band interleaves modules
 * (`ai/fn_802D0DCC.c` before them, `ef/fn_803066F0.c` after), so no `include/unsplit/<module>.h`
 * can own them (docs/plan.md 6.5 rule 2's named gap).  Declared here, once. */
#ifdef __cplusplus
extern "C" {
#endif
void fn_802DE578(_PLW* self, u16* sub);
void fn_802F39DC(_PLW* self, s32 arg);
#ifdef __cplusplus
}
#endif

/* The unit's frame windows, unclaimed `.sdata2` constants (playbook 23): declared, never defined, so
 * the load operands pair with the map's pool by name. */
extern const f32 lbl_80799E00;
extern const f32 lbl_80799ECC;
extern const f32 lbl_80799ED0;
extern const f32 lbl_80799ED4;
extern const f32 lbl_80799ED8;
extern const f32 lbl_80799EDC;
extern const f32 lbl_80799EE0;
extern const f32 lbl_80799EE8;

/* Advances one step of the act the player entered from an off-hand gesture.  The act step byte
 * picks between the arming step and the per-skill follow-up steps; the two constants the follow-up
 * steps hand `Pl_frame_check` only differ per gesture, so each arm is written out in full. */
extern "C" void fn_8024F200(_PLW* self, s32 part)
{
    s32 motion;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (part == 0) {
            fn_80275B04(self, 0, 0, 0);
            fn_8026A224(self, 313, 4, 0);
        } else {
            fn_80275B04(self, 3, 0, 0);
            fn_8026A224(self, 356, 4, 0);
        }
        self->field_0x018 = 0;
        fn_80277C58(self);
        self->field_0x007 = 0;
        if (Pl_cat_skill_ck(self, 50) == 1) {
            s32 level = self->field_0x0B6 & 7;

            if (level == 0) {
                self->field_0x007 = 1;
            } else if (level <= 2) {
                self->field_0x007 = 2;
            } else {
                self->field_0x007 = 3;
            }
        }
        if (Pl_Skill_ck(self, 32) == 1) {
            self->field_0x007++;
        }
        break;
    case 1:
        if (Pl_Skill_ck(self, 31) == 1 || self->field_0x007 == 1) {
            if (Pl_frame_check(self, 0, lbl_80799ECC, lbl_80799E00) == 1) {
                if (part == 0) {
                    fn_8026A224(self, 313, 0, 238);
                } else {
                    fn_8026A224(self, 356, 0, 238);
                }
            }
        } else if (self->field_0x006 == 0) {
            switch (self->field_0x007) {
            case 2:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, lbl_80799ED0, lbl_80799E00) == 1) {
                        self->field_0x006++;
                        fn_8026A224(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, lbl_80799ED0, lbl_80799E00) == 1) {
                        self->field_0x006++;
                        fn_8026A224(self, 356, 0, 238);
                    }
                }
                break;
            case 3:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, lbl_80799ED4, lbl_80799E00) == 1) {
                        self->field_0x006++;
                        fn_8026A224(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, lbl_80799ED4, lbl_80799E00) == 1) {
                        self->field_0x006++;
                        fn_8026A224(self, 356, 0, 238);
                    }
                }
                break;
            case 0:
                if (Pl_Skill_ck(self, 32) == 1) {
                    if (part == 0) {
                        if (Pl_frame_check(self, 0, lbl_80799ED8, lbl_80799E00) == 1) {
                            self->field_0x006++;
                            fn_8026A224(self, 313, 0, 200);
                        }
                    } else {
                        if (Pl_frame_check(self, 0, lbl_80799EDC, lbl_80799E00) == 1) {
                            self->field_0x006++;
                            fn_8026A224(self, 356, 0, 198);
                        }
                    }
                }
                break;
            }
        }
        if (Pl_frame_check(self, 0, lbl_80799EE0, lbl_80799E00) == 1) {
            if (fn_8027BC48(1) == 0) {
                fn_80272E30(self, self->field_0x306, -1);
                switch (self->field_0x306) {
                case 98:
                case 207:
                    motion = 150;
                    break;
                case 48:
                    motion = 100;
                    break;
                }
                fn_80278674(self, motion, 0);
            }
        }
        if (fn_8026A33C(self) == 1) {
            fn_802761B8(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* Advances the act the player enters when it is caught by the pitfall state: arms the three motion
 * words from the work record's own frame data, then waits for the step's frame window (or for the
 * master action to finish) before handing the act to `fn_802F39DC`. */
extern "C" void fn_8024F8A8(_PLW* self)
{
    u8 step;

    fn_802DE578(self, &self->field_0x598);
    fn_80277C48(self, 2);
    step = self->act_step_0x05;
    switch (step) {
    case 0:
        self->act_step_0x05 = step + 1;
        fn_8026A224(self, 338, 4, 0);
        fn_80275B04(self, 0, 0, 0);
        fn_8026FEC0(self, 2048);
        self->field_0x018 = 0;
        self->field_0x596 = 1;
        self->field_0x59A = 0;
        self->field_0x597 = 0;
        self->field_0x59C[0] = 0;
        self->field_0x59C[1] = 0;
        self->field_0x59C[2] = 0;
        break;
    case 1:
    case 2:
        if (fn_8026A33C(self) == 1) {
            fn_80275AC4(self, 0, 64, 0);
        } else if (Pl_frame_check(self, 0, lbl_80799EE8, lbl_80799E00) == 1) {
            if (self->act_step_0x05 == 1) {
                self->act_step_0x05++;
                fn_802F39DC(self, 0);
            }
        }
        break;
    default:
        break;
    }
}

/* Advances the same pitfall act's follow-up: it hands the act back to the action the three armed
 * motion words select, and stops for the frame window that the work record's own data names. */
extern "C" void fn_8024F9C4(_PLW* self)
{
    s32 i;

    fn_802DE578(self, &self->field_0x598);
    fn_80277C48(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_80275B04(self, 0, 0, 0);
        fn_8026FEC0(self, 2048);
        self->field_0x018 = 0;
        fn_8026A224(self, 339, 4, 0);
        break;
    case 1:
        if (self->field_0x59A != 0) {
            for (i = 0; i < 3; i++) {
                if ((self->field_0x59C[i] & 0x8000) != 0) {
                    fn_80275AC4(self, 0, 154, 0);
                    return;
                }
            }
        }
        if (fn_8026A644(self, 9) == 1) {
            if (self->field_0x59A != 0) {
                fn_80275AC4(self, 0, 65, 0);
            } else if (self->field_0x596 == 0) {
                fn_80275AC4(self, 0, 66, 0);
            } else {
                fn_80275AC4(self, 0, 67, 0);
            }
        }
        break;
    }
}

/* Counts how many of the three motion words the work record still has armed: 1, or 2 when the first
 * two agree, plus one more when the third matches the first. */
extern "C" s32 fn_8024FB20(_PLW* self)
{
    s16 count = 1;

    if (self->field_0x59C[0] == self->field_0x59C[1]) {
        count = 2;
    }
    if (self->field_0x59C[0] == self->field_0x59C[2]) {
        count = count + 1;
    }
    return count;
}

/* Resets the three guard timers the act cluster counts down: the two running timers to zero and the
 * third to its 30-frame window. */
extern "C" void fn_80257E70(_PLW* self)
{
    self->field_0x400 = 0;
    self->field_0x3FC = 0;
    self->field_0x402 = 30;
}
