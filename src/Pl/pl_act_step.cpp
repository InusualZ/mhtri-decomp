/*
 * Pl/pl_act_step.cpp - the player's per-act state machine cluster.
 *
 * Proposal `8024F200_fn_8024F200`, registered once at its final home (`docs/plan.md` 12).  `.text`
 * 0x8024F200-0x80258FCC (40396 B, 92 functions): every function takes the player work
 * (`_PLW*`, passed straight to `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no` and
 * to the `Pl/pl_act.cpp` motion helpers) plus the per-part index, reads the act step byte
 * `_PLW+0x005`, and drives one step of that act.  The unit's own dispatcher is its 0x80251B88
 * (1696 B, unread): it indexes `jumptable_805C4AE0` with `_PLW+0x00C` and tail-calls one handler per
 * act id, which is where this range's functions are reached from.  The whole range is worked as one
 * unit: the discovery pass cut it on its `--max-bytes` cap, and nothing inside the range names a
 * file (the `.data` pool between `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at
 * 0x805CDFC8 carries no `__FILE__` string for the whole Pl band), so no seam can be shown from the
 * range itself.
 *
 * Home (evidence order): 1. no `__FILE__` string anywhere in the band (see above); 2. no
 * runtime-dump name - `dumpmap.py lookup 0x8024F200` answers only the `zz_024f200_` placeholder;
 * 3. class 3 - the first argument is the `_PLW` every `Pl_*` predicate in this lib takes, and the
 * callees are `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no`, so the module is
 * `Pl`; 4. class 4 for the stem - the map carries only `fn_XXXXXXXX` for the range and the dump
 * answers `zz_`, so the name is the one derived below and is a **GUESS**.
 *
 * Name (GUESS, class 4).  `pl_act_step` says what the range is: the per-act step handlers the act
 * dispatcher tail-calls, each advancing one step of the act `_PLW+0x00C` names through
 * `_PLW+0x005`.  The naming scheme is the module's own (`pl_act.cpp`, `pl_skill.cpp`) with the
 * `_step_` segment naming the per-act entry each body switches on.
 *
 * Names this unit defines (all GUESSes, class 4, derived from each body - the map has no name for
 * any of them and `dumpmap.py` answers `zz_024f200_` for the whole range):
 *   * `pl_act_step_offhand_gesture` (0x8024F200) - drives the act entered from an off-hand gesture
 *     (its own body picks the skill tier out of `+0x0B6` and the `Pl_cat_skill_ck(self, 50)` gate);
 *   * `pl_act_step_pitfall_arm` (0x8024F8A8) - arms the pitfall act's three motion words from the
 *     work record's frame data, then hands the act to `fn_802F39DC` when its step 2 arrives;
 *   * `pl_act_step_pitfall_hold` (0x8024F9C4) - the pitfall act's follow-up: it hands the act back
 *     to the action the three armed words select and stops on the frame window they name;
 *   * `pl_act_armed_motion_count` (0x8024FB20) - counts how many of the three motion words agree
 *     (1, 2, or 3);
 *   * `pl_act_guard_timer_reset` (0x80257E70) - zeroes the two running guard timers and re-arms the
 *     third at 30 frames.
 *   * round 2, all class-4 GUESSes: `pl_act_step_84` (0x802504BC) - act 84, the only act id the
 *     dispatcher table gives that address; `pl_act_guard_gauge_adjust` (0x80252AEC) - steps
 *     `_PLW+0x0AC` (the guard gauge `pl_act_step_89` also clears) by the caller's two part flags and
 *     clamps it; `pl_act_arm_motion_and_flag` (0x802552F0) - arms a motion plus one of two flags and
 *     the actor mode, shared by the eleven weapon-act handlers above it; `pl_act_charge_repeat_step`
 *     (0x802574E4) - runs one repeat of the caller's six-part charge motion; `pl_act_countdown_step`
 *     (0x80257DA4) - arms attribute 26 with a 40/80-frame countdown; `pl_act_step_attr_1007` /
 *     `_1056` / `_1018` (0x8025553C / 0x80255E08 / 0x802562EC) and `pl_act_step_attr_112`
 *     (0x80253FF8) - the three-step weapon-act handlers, named from the attribute their arming step
 *     passes `Pl_chr_set_attr_default` (they are not in the act dispatch table; `fn_802564B0`'s
 *     `jumptable_805C4EB0` is what reaches the first three).
 *
 * Names this unit renamed in other units (the band's core API; every one is a **GUESS** derived from
 * the callee's own body, and the owner's header carries the declaration - rule 2):
 *   * `Pl_chr_set_attr_default` (0x8026A224, `Pl/fn_802693C4.cpp`) - `clrlwi r4,r4,16; li r7,0; b
 *     Pl_chr_set_attr`, i.e. the 5-argument attribute setter with its last argument 0;
 *   * `Pl_motion_end_ck` (0x8026A33C, `Pl/fn_802693C4.cpp`) - `fn_800E2198(&models_0x004[0], 0)`, the
 *     model layer's "the current motion finished" predicate its callers test `== 1` before arming
 *     the next one;
 *   * `Pl_act_set_motion` (0x80275B04, `Pl/fn_80273B14.cpp`) - arms the act's three status bits from
 *     a packed mask (1/2/3 through `fn_80275AFC`, then the two bit tests);
 *   * `Pl_act_set_motion_slot` (0x802761B8, `Pl/fn_80273B14.cpp`) - `li r7,0; li r8,0; b fn_80275C34`,
 *     the same hand-off with the two trailing arguments 0;
 *   * `Pl_act_set_step_table` (0x802770E8, `Pl/pl_act.cpp`) - installs the per-act `.data` record at
 *     `+0x318` and clears the `+0x313`/`+0x322` state it drives.
 * The four declarations those names had in `include/hud/fn_80334568.h` moved to their owner's header
 * in the same change (the hud unit includes it), and `include/unsplit/Pl.h`'s copies are gone.
 *
 * Language: C++ - the map's undefined set carries real manglings (`Pl_Skill_ck__FP4_PLWUs`,
 * `Pl_cat_skill_ck__FP4_PLWUs`, `Pl_frame_check__FP4_PLWUlff`, `GetGroundHit2__FPQ34nw4r4math4VEC3UlUcPUc`)
 * and the band's objects carry extab/extabindex (the Pl lib's `-Cpp_exceptions on`).
 *
 * Flags: `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw_version Wii/1.0),
 * the set the sibling Pl units measure with.  The per-part argument is `s32`, not `u8`: retail
 * compares it with `cmpwi r4,0` and never masks it, which a `u8` declaration cannot produce.
 *
 * Residual (20 of the range's 92 functions are written; the other 72 have no body yet).  Measured
 * with `unitscore.py Pl/pl_act_step`: unchanged by the naming pass (a rename moves an objdiff score
 * only through a relocation name).
 *   * `pl_act_step_offhand_gesture` 99.91 - `.text` (260 insns), `extab` and `extabindex` are byte-identical; the
 *     only gap is a *name*: the target's one `R_PPC_ADDR32` in extabindex points at dtk's
 *     `@etb_80011EFC` where MWCC writes its own anonymous `@283` local (the same class of residual
 *     `Pl/fn_80230FBC.cpp` records for its jump tables).  Nothing in the source reaches it.
 *   * `pl_act_step_pitfall_arm` 98.59 - one extra `clrlwi r0,r0,24` on the case-0 store.  The switch needs a
 *     `u8` local for retail's 3-instruction range test (`addi r0,r3,-1; cmplwi r0,1; ble`); the
 *     `u8` local's own conversion is what the `clrlwi` is, and both alternatives lose the range test
 *     instead (retail's 2-instruction store with the field written `++` in place of the local, or a
 *     `(u8)` cast on the switch operand, both measure 95.35).
 *   * `pl_act_step_pitfall_hold` 97.70 - 8 B short: MWCC removes the loop's counter (`li r4,0` / `addi r4,r4,1`
 *     are absent, and the address becomes `lhz r0,1436(r3)` off retail's own base) where retail
 *     keeps it.  The shape that keeps the counter (an explicit `u16*` plus `i`, `i++, word++`)
 *     changes the base to `addi r3,r31,1436` and measures 97.00, so the first shape is kept.
 *   * `pl_act_armed_motion_count` 100.00 and `pl_act_guard_timer_reset` 100.00.
 *   * the seven bodies added with the naming pass are all at 100.00 (`pl_act_step_86` 184 B,
 *     `pl_act_step_89` 280 B, `pl_act_step_94` 212 B, `pl_act_step_175` 200 B, `pl_act_step_151`
 *     168 B, `pl_act_step_timer_wait` 172 B, `pl_act_gauge_gate_by_skill` 84 B).
 *   * the eleven further bodies added in round 2 are all at 100.00: `pl_act_step_84` (272 B, act 84),
 *     `pl_act_step_attr_112` (308 B), `pl_act_guard_gauge_adjust` (296 B), `pl_act_arm_motion_and_flag`
 *     (152 B), `pl_act_charge_repeat_step` (596 B), `pl_act_countdown_step` (204 B), `pl_act_step_attr_1007`
 *     (144 B), `pl_act_step_attr_1056` (144 B), `pl_act_step_attr_1018` (144 B), plus the two the
 *     previous pass wrote and held back for want of a pool word - `pl_act_step_121` (348 B, act 121)
 *     and `pl_act_step_135` (396 B, acts 135/137).
 *   * **the Pl band's shared `.sdata2` pool now has a named owner**, which is what unblocked every
 *     body in round 2.  Rule 12 counts findings per (rule, file) and all eight of this file's pool
 *     `extern`s resolved to `unsplit`, so its count was 8 and **any** new pool declaration was
 *     refused by construction.  Route taken: the run `0x80799E00..0x80799F98` (408 B, the exact
 *     extent of the map's `pl_*` `.sdata2` rows) is claimed by the **data-only unit
 *     `Pl/pl_frame_data.cpp`**, and this unit reads the words through its header
 *     (`Pl/pl_frame_data.h`, included above) instead of declaring them locally.  That is the right
 *     shape rather than this unit holding the claim itself: the run is the MWLD *merge* of several
 *     Pl objects' own pools (`Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`, `Pl/fn_80258FCC.cpp`,
 *     `Pl/fn_8025F088.cpp` and this unit all read the same addresses - `callers.py 0x80799E40`
 *     answers 7 sites in 5 functions across 3 objects), so no single consumer can own or emit it,
 *     and the owner unit gives every one of those ~20 consumers somewhere to declare into.
 *     Measured, both when this unit held the claim and when it moved to the owner unit: **every row
 *     of the whole-project report is unchanged** (the 408 B of `total_data` simply changes unit; the
 *     only new report rows are the owner unit and the tail auto unit `auto_11_80799F98_sdata2` the
 *     re-split creates) and `ninja build/RMHE08/ok` stayed green.  Residual: the pool is *declared*,
 *     never defined (playbook 29 - and the owner unit's source intentionally defines nothing), so
 *     `datagap.py --unit Pl/pl_frame_data` reports `.sdata2 target-extra 408 B`: the bytes are the
 *     original's and a `NonMatching` unit contributes exactly those to the link.
 *   * `include/pl.h` gained **named union members only** (`field_0x406/408/40A/40C/410/422`,
 *     `field_0x42E/430/432/434/436`) for the timer run `pl_act_step_84` clears; headers carry rules
 *     2/12 only, so no rule 5 finding is created, the `_PLW` layout is unchanged and no consumer's
 *     codegen moves (the whole-project report was diffed row by row: 0 changes).
 *   * **one further body is written and measured but still held back**: `pl_act_step_arm_table`
 *     (176 B, 100.00 % measured) needs the `.data` step record `pl_act_step_table_247`
 *     (`.data:0x805BEBEC`), which is unowned, unclaimed and *outside* the Pl pool run the owner unit
 *     holds - declaring it here is a new rule-12 finding, and this batch has already spent the
 *     headroom a claim gives.  Its source is in `.pi/notes/pl-step2-ed81.md`; claiming the `.data`
 *     run it lives in is a separate measured change (filed as a `range` request in this unit's
 *     outbox).
 *   * the remaining `.data` labels the range reads (`lbl_805BDC48`, `lbl_805BDF78`, `lbl_805BEE14`,
 *     `lbl_805C4AD0`, the three `jumptable_805C4Axx`/`805C4DC4`/`805C4EB0` and
 *     `pl_act_step_table_247`) are unowned and unclaimed, which is what blocks the handlers that use
 *     them (74 rows still open).
 *   * `pl_act_charge_repeat_step`'s `part <= 2` test is `(u32)part <= 2`: the parameter stays `s32`
 *     (retail's inner 6-arm `switch` uses `cmpwi`, which a `u32` parameter turns into `cmplwi` and
 *     costs 2 points), but the range test itself is unsigned in retail (`cmplwi r30,2`).
 *   * a **new `fn_` name is still a rule-7 finding**, so every remaining body needs its callee named
 *     first (`Pl_motion_input_ck` alone blocks 9 of the 74 open rows).
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
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 float pool (rule 2) */
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h" /* the owner header of the act-motion setters (rule 2) */
#include "Pl/fn_80262940.h" /* the owner header of the model-state setter */
#include "ef/fn_800CDB2C.h"   /* the ef play-mode dispatcher the act-175 arm drives */
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

/* The frame windows this act cluster gates its frame checks on are this unit's *use* of the Pl band's
 * shared `.sdata2` pool; the declarations live in their owner's header (`Pl/pl_frame_data.h`, included
 * above) and the range is claimed by the data-only unit `Pl/pl_frame_data.cpp` (rule 2 + rule 12). */

/* Advances one step of the act the player entered from an off-hand gesture.  The act step byte
 * picks between the arming step and the per-skill follow-up steps; the two constants the follow-up
 * steps hand `Pl_frame_check` only differ per gesture, so each arm is written out in full. */
extern "C" void pl_act_step_offhand_gesture(_PLW* self, s32 part)
{
    s32 motion;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (part == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 313, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 356, 4, 0);
        }
        self->field_0x018 = 0;
        pl_act_set_frame_timer(self);
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
            if (Pl_frame_check(self, 0, pl_frame_window_96, pl_float_zero) == 1) {
                if (part == 0) {
                    Pl_chr_set_attr_default(self, 313, 0, 238);
                } else {
                    Pl_chr_set_attr_default(self, 356, 0, 238);
                }
            }
        } else if (self->field_0x006 == 0) {
            switch (self->field_0x007) {
            case 2:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 3:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 0:
                if (Pl_Skill_ck(self, 32) == 1) {
                    if (part == 0) {
                        if (Pl_frame_check(self, 0, pl_frame_window_246, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 313, 0, 200);
                        }
                    } else {
                        if (Pl_frame_check(self, 0, pl_frame_window_238, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 356, 0, 198);
                        }
                    }
                }
                break;
            }
        }
        if (Pl_frame_check(self, 0, pl_frame_window_260, pl_float_zero) == 1) {
            if (Pl_motion_input_ck(1) == 0) {
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
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* Advances the act the player enters when it is caught by the pitfall state: arms the three motion
 * words from the work record's own frame data, then waits for the step's frame window (or for the
 * master action to finish) before handing the act to `fn_802F39DC`. */
extern "C" void pl_act_step_pitfall_arm(_PLW* self)
{
    u8 step;

    fn_802DE578(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    step = self->act_step_0x05;
    switch (step) {
    case 0:
        self->act_step_0x05 = step + 1;
        Pl_chr_set_attr_default(self, 338, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
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
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 64, 0);
        } else if (Pl_frame_check(self, 0, pl_frame_window_104, pl_float_zero) == 1) {
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
extern "C" void pl_act_step_pitfall_hold(_PLW* self)
{
    s32 i;

    fn_802DE578(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 339, 4, 0);
        break;
    case 1:
        if (self->field_0x59A != 0) {
            for (i = 0; i < 3; i++) {
                if ((self->field_0x59C[i] & 0x8000) != 0) {
                    pl_act_enter(self, 0, 154, 0);
                    return;
                }
            }
        }
        if (pl_part_flag_ck(self, 9) == 1) {
            if (self->field_0x59A != 0) {
                pl_act_enter(self, 0, 65, 0);
            } else if (self->field_0x596 == 0) {
                pl_act_enter(self, 0, 66, 0);
            } else {
                pl_act_enter(self, 0, 67, 0);
            }
        }
        break;
    }
}

/* Counts how many of the three motion words the work record still has armed: 1, or 2 when the first
 * two agree, plus one more when the third matches the first. */
extern "C" s32 pl_act_armed_motion_count(_PLW* self)
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

/* Advances act 151's step: the arming step clears the actor mode, arms motion 113 with attribute
 * 2 and drops the act's two latch flags; the follow-up step hands the motion on once the model
 * reports it finished. */
extern "C" void pl_act_step_151(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 113, 2, 0);
        pl_act_clear_flag5bb(self);
        pl_act_clear_mode5c4(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}


/* Arms the act's 15-frame `+0x028` timer together with motion 1 and attribute 8/10, then counts the
 * timer down while the master gate holds and hands the motion on when it reaches zero.  Tail-called
 * by `pl_act_step_arm_hold`. */
extern "C" void pl_act_step_timer_wait(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 15;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 8, 10, 0);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (--self->field_0x28 <= 0) {
                Pl_act_set_motion_slot(self, 1, 4, 0);
            }
        }
        break;
    }
}

/* Picks the guard gauge band the cat-skill arm selects: skill 20 narrows it by one step, its absence
 * by two. */
extern "C" void pl_act_gauge_gate_by_skill(_PLW* self)
{
    if (Pl_cat_skill_ck(self, 20) == 1) {
        pl_act_gauge_gate(self, -1);
    } else {
        pl_act_gauge_gate(self, -2);
    }
}

/* Advances act 86's step: the arming step rotates the actor's +0x058/+0x0A8 angle pair by half a
 * turn (a `u16` wrap) and arms motion 329; the follow-up hands the act's motion on once the model
 * reports it finished. */
extern "C" void pl_act_step_86(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x058 = (u16)(self->field_0x058 + 0x8000);
        self->field_0x0A8 = self->field_0x058;
        Pl_chr_set_attr_default(self, 329, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* Advances act 94's step: the arming step arms motion 360 with attribute 4, and the second step
 * waits out the master gate and the act's own 20-frame window before bumping the act's follow-up
 * stage and handing the motion on. */
extern "C" void pl_act_step_94(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 360, 4, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && self->field_0x006 == 0) {
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->field_0x006++;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* Advances act 175's step: the arming step clears the actor mode, restarts the move work and arms
 * motion 307 with attribute 4, and the follow-up step hands the act its motion 1 once the model
 * reports the current one finished. */
extern "C" void pl_act_step_175(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 307, 4, 0);
        ef_move_state_dispatch(0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 1, 4, 0);
        }
        break;
    }
}

/* Advances acts 89 and 90's step: the arming step sets the actor's flag bit, arms motion 3, clears
 * the two rotation words and picks the motion attribute from the actor mode (102/117 clean, 1065/1067
 * when the mode byte is set), both keyed on the caller's part index; the follow-up hands the motion
 * on once the model reports it finished. */
extern "C" void pl_act_step_89(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_arm_flags(self, 1);
        Pl_act_set_motion(self, 3, 0, 0);
        self->param_0x54 = 0;
        self->field_0x0AC = 0;
        if (self->field_0x018 == 0) {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 102, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 117, 0, 0);
            }
        } else {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 1065, 8, 0);
            } else {
                Pl_chr_set_attr_default(self, 1067, 8, 0);
            }
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 8, 0);
        }
        break;
    }
}



/* Advances act 121's step: the arming step restarts the actor's move work, arms motion 318 with
 * attribute -4 and starts the act's 60-frame `+0x028` countdown, which the function decrements on
 * every call; the second step waits out the act's 46-frame window and arms motion 345; the third
 * waits for the master gate with the countdown expired before handing the model state on, and hands
 * the motion on once the model reports it finished. */
extern "C" void pl_act_step_121(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    if (self->field_0x28 != 0) {
        self->field_0x28--;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 318, -4, 0);
        self->field_0x28 = 60;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_46, pl_float_zero) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 345, 0, 0);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && self->field_0x28 == 0 && self->field_0x006 == 0) {
            self->field_0x006++;
            pl_model_state_set(self, 1, 25, 0);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    default:
        break;
    }
}

/* Advances acts 135 and 137's step: the arming step restarts the motion and arms attribute 320 with
 * the actor's flag word (or 324 alone for the second part); the follow-up waits out the act's
 * 44-frame window and its part flag before re-entering the act, and the second part counts the same
 * window up until the act's own two frames have passed. */
extern "C" void pl_act_step_135(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        if (part == 0) {
            Pl_chr_set_attr_default(self, 320, 4, 0);
            pl_act_set_flag(self, 512);
        } else {
            Pl_chr_set_attr_default(self, 324, 4, 0);
        }
        break;
    case 1:
        switch (part) {
        case 0:
            if (self->field_0x006 >= 3 ||
                (Pl_frame_check(self, 1, pl_frame_window_44, pl_float_zero) == 1 &&
                 pl_part_flag_ck(self, 4) == 1)) {
                pl_act_enter(self, 0, 136, 0);
            } else if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                self->field_0x006++;
            }
            break;
        case 1:
            if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                if (++self->field_0x006 >= 2) {
                    pl_act_enter(self, 0, 138, 0);
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* Advances the weapon act whose arming step carries attribute 112: the arming step arms motion 3
 * with flag 64 and copies the actor's second counter into its first; the follow-up step enters act
 * 8 out of its 56-frame window while the act's tier still reads 0, and the unmatched path falls
 * back to entering act 7 once the model reports the motion finished. */
extern "C" void pl_act_step_attr_112(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 64);
        self->field_0x058 = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 112, 0, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1 &&
                pl_act_param_tier_ck(self, 0) == 0) {
                pl_act_enter(self, 1, 8, 0);
            } else if (Pl_motion_end_ck(self) == 1) {
                pl_act_enter(self, 1, 7, 0);
            }
        } else if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1) {
            pl_act_enter(self, 1, 8, 0);
        }
        break;
    default:
        break;
    }
}

/* Resets the three guard timers the act cluster counts down: the two running timers to zero and the
 * third to its 30-frame window. */
extern "C" void pl_act_guard_timer_reset(_PLW* self)
{
    self->field_0x400 = 0;
    self->field_0x3FC = 0;
    self->field_0x402 = 30;
}

/* Advances act 84's step: restarts the actor's move work and re-arms the act's whole timer block -
 * the stagger timer, the stamina/guard pair, the two four-word runs and the act bitfield - then
 * hands the act to 85 once the model reports the motion finished. */
extern "C" void pl_act_step_84(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 326, -6, 0);
        self->field_0x3EA = 0;
        pl_act_clear_wait(self, 0);
        self->field_0x3FC = 0;
        self->field_0x42E = 0;
        self->field_0x430 = 0;
        self->field_0x432 = 0;
        self->field_0x434 = 0;
        self->field_0x436 = 0;
        self->field_0x422 = 0;
        self->field_0x404 = 0;
        self->field_0x406 = 0;
        self->field_0x408 = 0;
        self->field_0x40A = 0;
        self->field_0x40C = 0;
        self->field_0x3DC = 0;
        self->field_0x410 = 0;
        self->field_0x3F4 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 85, 0);
        }
        break;
    default:
        break;
    }
}

/* Steps this act's guard gauge by the caller's two part flags and clamps it into the act's window.
 * The first flag takes 1024 off the gauge unless the menu owns the screen, the second puts 1024
 * back, and neither leaves the gauge to walk back toward zero in 768 steps; the result is then
 * clamped to the caller's two limits, held in the same 16-bit wrap the field itself uses. */
extern "C" void pl_act_guard_gauge_adjust(_PLW* self, u16 neg_limit, u16 pos_limit, s32 flag_a,
                                          s32 flag_b)
{
    u32 gauge;

    if (pl_part_flag_ck(self, flag_a) == 1 && Pl_suimen_ck(self) == 0) {
        self->field_0x0AC -= 1024;
    } else if (pl_part_flag_ck(self, flag_b) == 1) {
        self->field_0x0AC += 1024;
    } else {
        gauge = self->field_0x0AC;
        if ((s16)gauge >= 0) {
            if (gauge >= 768) {
                self->field_0x0AC = gauge - 768;
            } else {
                self->field_0x0AC = 0;
            }
        } else {
            if (gauge >= 64768) {
                self->field_0x0AC = 0;
            } else {
                self->field_0x0AC = gauge + 768;
            }
        }
    }
    gauge = self->field_0x0AC;
    if ((s16)gauge < 0) {
        if ((u16)gauge <= neg_limit) {
            self->field_0x0AC = neg_limit;
        }
    } else if ((u16)gauge >= pos_limit) {
        self->field_0x0AC = pos_limit;
    }
}

/* Arms one motion of the caller's part, sets the act's low or high flag from the second argument,
 * marks the actor mode, and re-arms the act's flag set unless the caller says otherwise.  Shared by
 * the whole weapon-act band above 0x80255388, which is why its arguments stay generic. */
extern "C" void pl_act_arm_motion_and_flag(_PLW* self, u16 motion, u8 high_flag, u8 skip_arm)
{
    Pl_act_set_motion(self, motion, 0, 0);
    if (high_flag == 0) {
        pl_act_set_flag(self, 16);
    } else {
        pl_act_set_flag(self, 80);
    }
    self->field_0x018 = 1;
    if (skip_arm == 0) {
        pl_act_arm_flags(self, 1);
    }
}

/* Runs one repeat of the charge act the caller's part selects: the arming step picks the motion
 * (0 for the low parts, 3 for the high ones) and the attribute (211 or 263) together with the
 * repeat count the part sets at `+0x007`; the follow-up step waits out that many motion-end reports
 * before re-entering the act, and the last step hands the act's own motion slot on. */
extern "C" void pl_act_charge_repeat_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        switch (part) {
        case 0:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 1:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 2:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 3:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 4:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 5:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        default:
            break;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (++self->field_0x006 >= self->field_0x007) {
                self->act_step_0x05++;
                if ((u32)part <= 2) {
                    Pl_chr_set_attr_default(self, 212, 2, 0);
                } else {
                    Pl_chr_set_attr_default(self, 264, 2, 0);
                }
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, self->kind_0x09, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* Arms one of the act's countdown motions: the arming step clears the actor mode, arms attribute 26
 * with its frame timer and the motion, and starts a 40-frame countdown for the first part (80 for
 * the others); the follow-up hands motion slot 12 on once the countdown runs out. */
extern "C" void pl_act_countdown_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 26, 6, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        if (part == 0) {
            self->field_0x28 = 40;
        } else {
            self->field_0x28 = 80;
        }
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            Pl_act_set_motion_slot(self, 0, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* Advances the weapon act whose arming step carries attribute 1007: it arms that attribute, hands
 * the act's motion on through the shared part armer, and hands motion slot 2 on once the model
 * reports the motion finished. */
extern "C" void pl_act_step_attr_1007(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1007, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* The same three steps for the weapon act whose arming step carries attribute 1056, which arms
 * motion 3 instead of motion 0. */
extern "C" void pl_act_step_attr_1056(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1056, 2, 0);
        pl_act_arm_motion_and_flag(self, 3, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* And for the weapon act whose arming step carries attribute 1018: the follow-up does not arm the
 * act's own motion but enters the child act 5 instead. */
extern "C" void pl_act_step_attr_1018(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1018, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 5, 1, 0);
        }
        break;
    default:
        break;
    }
}
