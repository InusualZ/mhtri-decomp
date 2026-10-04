/*
 * hud/pl_frame_sync.cpp - phase 4 unit, `.text` 0x8033041C..0x80338808 (125 functions, 33772 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Fold of 2 registered units: em_pl_frame.cpp, net_char_sync.cpp.  The
 * functions below are the ones those sources define, in address order; every other function of the range keeps its
 * original bytes.  77 of 125 functions have a body here.
 *
 * FLAGS.  `cflags_hud` (net_char_sync's group); the absorbed enemy/em_pl_frame.cpp had no bodies.  The unit's pool
 * check is the candidate's inherited FAIL.
 *
 * GUESS (rule 7): the stem joins the two old names, the player frame checks (em_pl_frame) and the net character sync.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.data, .sdata, .sdata2, .text, extab, extabindex).
 */
/* ---- header inherited from src/enemy/em_pl_frame.cpp (written against its pre-phase-4 range) ---- */
/*
 * enemy/em_pl_frame.cpp - the second half of proposal/8032C920_fn_8032C920.cpp's range, `.text`
 * 0x8033041C..0x80334568 (48 functions, 16716 B).  Registered once, at its final home (docs/plan.md
 * 12): that proposal's range 0x8032C920..0x80334568 was one `--max-bytes` cut over **two** translation
 * units, and the seam re-draw of 2026-09-26 split it into `enemy/em_act_step.cpp` (first half, 74
 * functions / 15100 B) and this file.  No function is reconstructed yet - the file exists as the
 * registration the register-once rule requires, and its header says what the unit is, its range, why
 * it sits there, what is unknown and where the evidence lives.
 *
 * THE SEAM, MEASURED - three independent instruments agree on 0x8033041C:
 *
 *  - the extabindex run 0x800357A8..0x800359A0: an extabindex entry names its own function, and entry
 *    57 (at 0x800357A8) names `fn_8033041C`.  The run's 42 entries cover this unit's 42 framed
 *    functions; the first half takes the other 57 of the 99.
 *  - the `.sdata2` pool run 0x8079B108..0x8079B2AC is **two objects' pools**, cut at 0x8079B20C |
 *    0x8079B210: the first half's labels are referenced only by functions 0x8032C920..0x80330128, this
 *    unit's only by functions 0x803305F8..0x80334398, and no label is shared.  The duplicated
 *    constants are the proof: MWCC's u32->f32 magic `0x4330000080000000` is emitted at 0x8079B140
 *    (first half - `em_act_arm_mot5`, `em_act_arm_mot7`) and at 0x8079B228 (this unit - `fn_803305F8`), and
 *    0.0f (0x8079B108 / 0x8079B240), 0.5f (0x8079B1E4 / 0x8079B214), 1.0f (0x8079B130 / 0x8079B284),
 *    10.0f (0x8079B110 / 0x8079B238), 20.0f (0x8079B160 / 0x8079B21C), 30.0f (0x8079B12C /
 *    0x8079B218 + 0x8079B258), 60.0f (0x8079B168 / 0x8079B220), 0.8f (0x8079B178 / 0x8079B2A0) and
 *    -30.0f (0x8079B17C / 0x8079B294) each occur once per half.  A pool emits one copy of a constant
 *    per object - the first half's own object uses the magic in two functions and emits a single
 *    8-byte entry - and the linker merges nothing (that magic occurs >= 40x in the DOL's `.sdata2`),
 *    so two copies mean two emitters, i.e. two TUs.
 *  - the `.data` run: the first half's table fragment ends with the class vtable 0x805E04E0 (0x30 B,
 *    installed by `em_work_ctor`) and this unit's begins at 0x805E0510 with six objects that
 *    `fn_803305F8` alone references; the referrer addresses jump 0x8032FA88 -> 0x803305F8 exactly at
 *    that fragment edge.
 *
 * SECTIONS.  `.text` 0x8033041C..0x80334568; extab 0x8001662C..0x8001677C (42 x 8 B); extabindex
 * 0x800357A8..0x800359A0 (42 x 12 B).  Its `.sdata2` pool half is 0x8079B210..0x8079B2AC - 0x8079B210
 * (0.5), 0x8079B214 (0.5), 0x8079B218 (30.0), 0x8079B21C (20.0), 0x8079B220 (60.0), 0x8079B228 (the
 * u32->f32 magic, 8 B), 0x8079B230 (8 B, `0x03AA2425`), 0x8079B238 (10.0), 0x8079B23C (0.7),
 * 0x8079B240 (0.0), 0x8079B244 (-1.0), 0x8079B248.., 0x8079B258 (30.0), 0x8079B294 (-30.0),
 * 0x8079B2A0 (0.8) - and it stays **unclaimed**: a `.sdata2` claim links only while the object emits
 * no pool of its own (playbook 23/58), so the labels are declared and used as load operands only
 * (playbook 29).  Its `.data` run 0x805E0510..0x805E201C is deliberately **not claimed** either: the
 * next band's `Pl_act_step_table_enter` owns an object at 0x805E1ED0 inside that address range, so the run is not
 * claimable whole, and a run with an unclaimed hole in it becomes an `auto_*_data` unit in the middle
 * of the unit's range (playbook 53).  The measured object list, for the data pass that claims it:
 * 0x805E0510 / 0x805E0540 / 0x805E0570 (0x30 each), 0x805E05A0 (0x38), 0x805E05D8 (0xC),
 * jumptable_805E05E4 (0x50) - all `fn_803305F8`; 0x805E0638 / 0x805E0688 / 0x805E06D8 (0x50 each, no
 * referrer); 0x805E0728 (0xC) + 0x805E0734 (0x32C) - `fn_80331238`; then one object per function from
 * 0x805E0A60 (`pl_act_handler_00`) to 0x805E1E20 (`pl_act_handler_27`), plus 0x805E1F6C (`pl_act_handler_28`) and
 * 0x805E1F98 (`pl_act_handler_29`).
 *
 * MODULE AND NAME - the pass that writes the bodies decides, and this is the hint it starts from.  No
 * `__FILE__` string is reachable and `tools/symbols/dumpmap.py lookup` answers `zz_XXXXXXXX_` for every
 * address of the range, so the map has no name for the file either.  Module `enemy` is the link
 * band's: 0x803250B0..0x80334568 is the enemy band (`enemy/em_action.cpp` and
 * `enemy/em_act_step.cpp` bracket it) and the neighbouring files are `enemy/`.  The honest caveat, of
 * the same kind `hud/net_char_sync.cpp` records for its band: the content is the *player* work.  This
 * half drives the player work record (`_PLW` reached with `get_move_work_adrs(2)`, then
 * `Pl_frame_check` / `Get_motion_no` / `Pl_chr_setX` / `Pl_master_ck`, and the documented `+4` ->
 * `get_joint_wpos` idiom) and it owns a second class's table family (vtable 0x805E0510 plus
 * `fn_803305F8`'s six objects at 0x805E0510..0x805E0634).
 *
 * The name `em_pl_frame` is **provisional** - the merger lane's 2026-09-26 naming pass had to leave a
 * `fn_XXXXXXXX` file stem behind (the gate's rule 7 row exempts a bodyless unit from the rule itself,
 * but never a generated *path*), and the only evidence a name can come from before the bodies exist is
 * the target object's own call surface, read with `tools/units/m2cinput.py`: of its 48 functions, 31
 * call `Pl_frame_check__FP4_PLWUlff` (15 `Pl_master_ck__FP4_PLW`), and the range's heaviest callees are
 * all in the `Pl`/`player` band - `Pl_motion_end_ck` x44, `fn_803313D0` x40, `Pl_act_set_motion` x33,
 * `Pl_act_set_motion_slot` x24, `fn_80277C50` x24, `pl_act_enter` x16, `Pl_act_set_step_table` x10 - plus the effect spawns
 * `res_eft_create__FUsUsUl`, `res_eft_model_create__FP6MHcharUsUl` and `eft007_set__FP4_PLWUcUcUlPQ34nw4r4math4VEC3f`.
 * So the half runs the *player records'* frame checks and spawns the hit effects; the pass that writes
 * the bodies renames this file from them, and if a `Pl` home turns out to be right, this unit moves.
 * Its own 48 symbols keep the map's `fn_` stems until then (writing a body is what names it).
 *
 * FLAGS.  `cflags_main` (Wii/1.3, `-O3 -inline noauto -Cpp_exceptions on`), the first half's set: 42 of
 * its 48 functions are framed and carry an extab/extabindex record, which is what
 * `-Cpp_exceptions on` produces, and the band's callees are the same mangled C++ symbols.
 *
 * THE RECORD.  Nothing is written yet.  The first half's file keeps the `_EM_CHARA_WORK` view and
 * `include/unsplit/enemy.h` carries the first half's pool declarations only; this unit's pool labels
 * (listed under SECTIONS) are NOT declared there and have to be declared, as externs, in this unit's
 * own header when its bodies arrive.
 *
 * NEXT.  Write the 48 bodies in address order from `fn_8033041C` (0x1DC B, the per-attacker effect spawn
 * that continues `em_act_arm_mot201_hit7_2`'s scan shape): `python tools/units/m2cinput.py` for a draft,
 * then `python tools/units/recompile.py enemy/em_pl_frame.cpp --measure <symbol> --main .`, one function
 * per pass, best-scoring variant, never a regression.
 */
/* ---- header inherited from src/hud/net_char_sync.cpp (written against its pre-phase-4 range) ---- */
/* hud/net_char_sync.cpp - the character-state network sync (`.text` 0x80334568..0x80338808, 77 functions /
 * 17056 B, all written).  A sender packs a player work (`_PLW`), an enemy work (`_ENEMY_WORK`), the
 * enemy-control work (`EmcWork`) or an effect slot (`EftSlot`) into one local wire message and hands it to
 * `broadcastSessionCommand`; the matching receiver applies a message back.  Four families in address order:
 * the player family (`Pl_net_*`, kinds 1-9 of the message header, plus the act dispatcher and the two act
 * entries that open the range), the enemy family (`em_net_*`, kinds 1-7), the enemy-control family
 * (`emc_net_*`, kinds 1-2) and the effect-slot family (`eft_net_*`, modes 1-8).  Each message is one struct
 * in `include/hud/net_char_sync.h`; its constructor and `NetMsgHeader::fill` are real class members (their map
 * rows carry the manglings).
 *
 * Home, name and evidence:
 *   - no `__FILE__` string or `.data`/`.sdata` reference names the source file and the runtime dump answers
 *     `zz_`/`FUN_` for all 77 addresses, so the stem `net_char_sync` names the role (a GUESS; the unit was
 *     registered under its generated map stem).
 *   - module `hud`: `cflags_hud` is the only registered group whose flags reproduce the target bytes
 *     (`-opt nopeephole`, `-Cpp_exceptions on`: 67 extab/extabindex records, one per framed function).  The
 *     content is network, not HUD; if a `Network` lib becomes the home of the multiplayer sync, this unit is the
 *     first candidate to re-home.
 *   - the seam is unproven (`tudiscover.py at 0x80334568` must-links only the first function); the range is
 *     one maximal unclaimed run between `enemy/em_pl_frame.cpp` below and `lobby/lb_companion_ui.cpp` above.
 *
 * Sections this unit owns: .text, extab 0x8001677C..0x80016994, extabindex 0x800359A0..0x80035CC4 and .data
 * 0x805E2788..0x805E27D4 (the `Pl_net_recv` and `eft_net_send` switch tables).
 *
 * Flags: the lib's `cflags_hud`.  Score: 99.98 % fuzzy, 17056 of 17056 `.text` bytes, 73 of 77 bodies
 * byte-identical.
 *
 * Residuals (each is the same instruction count and size; only a register or a stack slot differs):
 *   - `em_net_send_count` 99.41 %: the zero stored to `sync_flag_0x468` and the switch value swap r0/r3.
 *   - `em_net_parts_pack` 99.48 %, `em_net_parts_unpack` 99.83 %: the target keeps the zero-extended part
 *     index in r4 across the damage-level call, ours recomputes it into r0.
 *   - `em_net_recv_target` 99.96 %: the three temporaries (two returned world positions and `delta`) sit in
 *     different stack slots.
 *   - `.data`: our object emits 432 B, the claim holds 76 B.  The act dispatcher's 0x164-byte switch table
 *     (`jumptable_805E0EA0`) is sole-owned but span-blocked (`enemy/em_pl_frame` reads `lbl_805E1004` between it
 *     and the claim); `lbl_805E1ED0` (0x805E1ED0, extern) is blocked the same way by
 *     `lbl_805E1F6C`.  `.sdata2` `lbl_8079B2B0` (the u16 -> f32 magic, 8 B) is a pool entry our object also
 *     emits, so it is not claimable (`pool-synth`).  The unit cannot flip to Matching until those two
 *     claims are possible.
 *
 * Data rows left as `lbl_` until the claim lands (span-blocked by enemy/em_pl_frame): `lbl_805E1ED0`.
 *
 * Source shapes that are load-bearing (measured):
 *   - `&get_worldworld_pos(...)` (with `const_cast<VEC3*>` on the `const` message position, a compile-only cast) as `copyVec3`'s source (MWCC warning 10142, "not an lvalue"): every other
 *     spelling copies the returned record through an extra temporary.
 *   - a `switch` whose `default:` comes first (`emc_net_recv_status`'s found-kind dispatch), and receivers that
 *     take a non-const message (a const message lets the compiler reuse a load across the stores).
 *   - `(s32)plw->field_0x002 == 7` (a plain `u8` compare emits `cmplwi`, the target `cmpwi`); `mask` as `u16`
 *     and the declaration order of the loop locals decide the register order in several bodies.
 *   - `_PLW` +0x01F/+0x64F/+0x583 are `u8` and +0x452 is `s8` (the target stores them without an `extsb`);
 *     `pl_act_stage_get` returns `u8` and `pl_act_stage_latch_set` takes one, `eft_slot_armed_ck` returns `u32`.
 *
 * Names are derived from the bodies and their callers; every name below that is not the map's or the dump's
 * is a GUESS:
 *   - the player family's roles (`send_state`/`_alt` = kinds 1/9, `send_pos` 2, `send_hit` 3/5, `send_extra` 6,
 *     `send_item` 7/8, `recv_item_use`/`recv_item_result`); the enemy family's kinds (`state`, `target`, `flags`,
 *     `act`, `count`, `bind`, `release`); `emc_*` and `eft_*` as above.
 *   - callees renamed with their owners' sources and headers swept: `pl_pos_blend_start` and `pl_act_add_charge`
 *     (their owner's own doc comments), `pl_act_enter_raw`, `pl_hit_effect_spawn`, `pl_act_net_hook_*`,
 *     `eft_rot_vec_copy`, `item_se_ck`, `se_slot_req`, `quest_enemy_spawn_req`, `eft_slot_marks_set`,
 *     `em_parts_*`, `em_event_settle*`, `em_get_part_limit`, `em_special_part_apply`, `em_act_*`, `em_area_change`,
 *     `em_status_*` (`em_status_set` is the function the map stores +0x43D through), `em_alt_mode_set` (the latch
 *     `em_alt_mode_ck` tests), `em_level_raise`, `em_net_flag_apply_0..7` (one per bit of the action-flag byte),
 *     `em_state_refresh`, `em_motion_mode_set` (its owner's doc comment), `em_break_state_ck`, `em_busy_ck`,
 *     `emc_mini_*`, `emc_marker_replay`, `root_mode2_ck` (its owner's doc comment), `pl_act_stage_get`; the thirty
 *     `pl_act_handler_NN` are the dispatcher's act handlers, numbered in address order (their bodies belong to
 *     the still-unwritten `enemy/em_pl_frame.cpp`).
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "nw4r/math.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef.h"
#include "ef/fn_800CDB2C.h"
#include "Pl/pl_act.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80273B14.h" /* the owner header of the act-motion setters (rule 2) */
#include "hud/net_char_sync.h"
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_80288CEC.h"
#include "ef/get_move_work_adrs.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_80262940.h"
#include "Pl/fn_802430E8.h"
#include "Pl/fn_802489D4.h"
#include "ef/eft001.h"
#include "sound/fn_800EF7D8.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8/item_se_ck.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80137604.h"
#include "enemy/em_pl_frame.h"
#include "stage/stg_w.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "ef/eft_slot.h"
#include "lobby/lb_quest_screen.h"

/* Advances the paired act-entry state machine: step 0 arms the act's attr and motion (the pair selects the
 * variant), step 1 swaps in the follow-up attr once the motion ends and step 2 hands the act on. */
void Pl_act_pair_enter(_PLW* plw, s32 pair) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x018 = 1;
        pl_act_clear_flag5bb(plw);
        plw->field_0x5C6 = 0;
        pl_act_add_charge(plw, 0x28);
        if (pair == 0) {
            Pl_chr_set_attr_default(plw, 0x45A, 4, 0);
            Pl_act_set_motion(plw, 0, 0, 0);
        } else {
            Pl_chr_set_attr_default(plw, 0x48C, 4, 0);
            Pl_act_set_motion(plw, 3, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            if (pair == 0) {
                Pl_chr_set_attr_default(plw, 0x475, 0, 0x28);
            } else {
                Pl_chr_set_attr_default(plw, 0x4A7, 0, 0x28);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, plw->kind_0x09, 4, 0);
        }
        break;
    }
}

/* Advances the act-change state machine: step 0 enters the act (its motion, its SE and the paired
 * `Pl_act_set_motion` step) and step 1 waits for the act's frame check before handing the act's motion on. */
void Pl_act_step_table_enter(_PLW* plw) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 0, 0);
        Pl_chr_set_attr_default(plw, 0x494, 4, 0x5A);
        Pl_act_set_step_table(plw, (u32)lbl_805E1ED0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        }
        break;
    }
}

/* Dispatches the player's act number (0..0x58) to the handler that serves it, passing the variant index the
 * act shares with its siblings; an act number without a handler re-enters the idle motion slot. */
void Pl_act_frame_dispatch(_PLW* plw) {
    switch (plw->act_no) {
    case 0x0:
        pl_act_handler_00(plw, 0);
        break;
    case 0x1:
        pl_act_handler_00(plw, 1);
        break;
    case 0x2:
        pl_act_handler_01(plw, 0);
        break;
    case 0x3:
        pl_act_handler_02(plw, 0);
        break;
    case 0x4:
        pl_act_handler_03(plw, 0);
        break;
    case 0x5:
        pl_act_handler_04(plw, 0);
        break;
    case 0x6:
        pl_act_handler_05(plw, 0);
        break;
    case 0x7:
        pl_act_handler_06(plw, 0);
        break;
    case 0x8:
        pl_act_handler_07(plw, 0);
        break;
    case 0x9:
        pl_act_handler_08(plw, 0);
        break;
    case 0xA:
        pl_act_handler_10(plw, 0);
        break;
    case 0xB:
        pl_act_handler_11(plw, 0);
        break;
    case 0xC:
        pl_act_handler_12(plw, 0);
        break;
    case 0xF:
        pl_act_handler_04(plw, 1);
        break;
    case 0x10:
        pl_act_handler_06(plw, 1);
        break;
    case 0x11:
        pl_act_handler_13(plw, 0);
        break;
    case 0x12:
        pl_act_handler_13(plw, 1);
        break;
    case 0x13:
        pl_act_handler_01(plw, 1);
        break;
    case 0x14:
        pl_act_handler_04(plw, 2);
        break;
    case 0x15:
        pl_act_handler_14(plw, 0);
        break;
    case 0x16:
        pl_act_handler_08(plw, 1);
        break;
    case 0x17:
        pl_act_handler_06(plw, 2);
        break;
    case 0x18:
        pl_act_handler_09(plw);
        break;
    case 0x19:
        pl_act_handler_15(plw);
        break;
    case 0x1A:
        pl_act_handler_06(plw, 3);
        break;
    case 0x1B:
        pl_act_handler_06(plw, 4);
        break;
    case 0x1C:
        pl_act_handler_01(plw, 2);
        break;
    case 0x1D:
        pl_act_handler_01(plw, 3);
        break;
    case 0x1E:
        pl_act_handler_05(plw, 1);
        break;
    case 0x1F:
        pl_act_handler_14(plw, 1);
        break;
    case 0x20:
        pl_act_handler_14(plw, 2);
        break;
    case 0x21:
        pl_act_handler_04(plw, 3);
        break;
    case 0x22:
        pl_act_handler_04(plw, 4);
        break;
    case 0x23:
        pl_act_handler_04(plw, 5);
        break;
    case 0x24:
        pl_act_handler_16(plw, 0);
        break;
    case 0x25:
        pl_act_handler_17(plw, 0);
        break;
    case 0x26:
        pl_act_handler_18(plw, 0);
        break;
    case 0x27:
        pl_act_handler_19(plw, 0);
        break;
    case 0x28:
        pl_act_handler_20(plw, 0);
        break;
    case 0x29:
        pl_act_handler_21(plw, 0);
        break;
    case 0x2A:
        pl_act_handler_22(plw, 0);
        break;
    case 0x2B:
        pl_act_handler_02(plw, 1);
        break;
    case 0x2C:
        pl_act_handler_00(plw, 2);
        break;
    case 0x2D:
        pl_act_handler_00(plw, 3);
        break;
    case 0x2E:
        pl_act_handler_23(plw, 0);
        break;
    case 0x2F:
        pl_act_handler_24(plw, 0);
        break;
    case 0x30:
        pl_act_handler_25(plw, 0);
        break;
    case 0x31:
        pl_act_handler_26(plw, 0);
        break;
    case 0x32:
        pl_act_handler_26(plw, 1);
        break;
    case 0x33:
        pl_act_handler_16(plw, 1);
        break;
    case 0x34:
        pl_act_handler_23(plw, 2);
        break;
    case 0x35:
        pl_act_handler_27(plw, 0);
        break;
    case 0x36:
        pl_act_handler_27(plw, 1);
        break;
    case 0x37:
        pl_act_handler_28(plw);
        break;
    case 0x38:
        pl_act_handler_29(plw, 0);
        break;
    case 0x39:
        pl_act_handler_24(plw, 1);
        break;
    case 0x3A:
        pl_act_handler_25(plw, 1);
        break;
    case 0x3B:
        pl_act_handler_25(plw, 2);
        break;
    case 0x3C:
        pl_act_handler_23(plw, 1);
        break;
    case 0x3D:
        pl_act_handler_23(plw, 3);
        break;
    case 0x3E:
        pl_act_handler_23(plw, 4);
        break;
    case 0x3F:
        pl_act_handler_23(plw, 5);
        break;
    case 0x40:
        pl_act_handler_17(plw, 1);
        break;
    case 0x41:
        pl_act_handler_17(plw, 2);
        break;
    case 0x42:
        pl_act_handler_17(plw, 3);
        break;
    case 0x43:
        pl_act_handler_17(plw, 4);
        break;
    case 0x44:
        pl_act_handler_16(plw, 2);
        break;
    case 0x45:
        pl_act_handler_16(plw, 3);
        break;
    case 0x46:
        Pl_act_pair_enter(plw, 0);
        break;
    case 0x47:
        Pl_act_pair_enter(plw, 1);
        break;
    case 0x48:
        pl_act_handler_08(plw, 2);
        break;
    case 0x49:
        pl_act_handler_14(plw, 3);
        break;
    case 0x4A:
        pl_act_handler_25(plw, 3);
        break;
    case 0x4B:
        Pl_act_step_table_enter(plw);
        break;
    case 0x4C:
        pl_act_handler_04(plw, 6);
        break;
    case 0x4D:
        pl_act_handler_04(plw, 7);
        break;
    case 0x4E:
        pl_act_handler_23(plw, 6);
        break;
    case 0x4F:
        pl_act_handler_23(plw, 7);
        break;
    case 0x50:
        pl_act_handler_04(plw, 8);
        break;
    case 0x51:
        pl_act_handler_23(plw, 8);
        break;
    case 0x52:
        pl_act_handler_05(plw, 2);
        break;
    case 0x53:
        pl_act_handler_10(plw, 1);
        break;
    case 0x54:
        pl_act_handler_29(plw, 1);
        break;
    case 0x55:
        pl_act_handler_24(plw, 2);
        break;
    case 0x56:
        pl_act_handler_08(plw, 3);
        break;
    case 0x57:
        pl_act_handler_27(plw, 2);
        break;
    case 0x58:
        pl_act_handler_27(plw, 3);
        break;
    default:
        Pl_act_set_motion_slot(plw, 0, 4, 0);
        break;
    }
}

/* Writes the 4-byte wire header every sender of this unit starts its message with. */
void NetMsgHeader::fill(u8 from, u8 to, u8 kind) {
    reserved = 0;
    from_slot = from;
    to_slot = to;
    this->kind = kind;
}

/* Reports whether this client may send player state: the local move work exists and either the player
 * is in a syncable act or the act layer says so. */
u32 Pl_net_can_send(void) {
    if (get_move_work_adrs(0) == NULL) {
        return 0;
    }
    if (root_mode2_ck() == 1) {
        return 1;
    }
    return Pl_motion_input_ck(0);
}

/* Builds and sends the 0x4C-byte player-state message: position, running act, health pair, armed-weapon
 * bytes, the skill point gauge and the four decoration-skill ids. */
void Pl_net_send_state(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlStateMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    msg.act_mode = plw->field_0x018;
    msg.field_0x04 = param;
    msg.pos = plw->vec_0x03C;
    msg.field_0x14 = plw->field_0x0AC;
    msg.field_0x18 = plw->field_0x0A8;
    msg.act_kind = plw->field_0x00A;
    msg.act_no = plw->act_no;
    msg.field_0x306 = plw->field_0x306;
    msg.sub_area = plw->sub_area_0x3B6;
    msg.held_item_kind = plw->held_item_kind_0x26C;
    msg.field_0xB6 = plw->field_0x0B6;
    if ((s32)plw->field_0x002 == 7) {
        msg.stagger = (u8)plw->field_0x386;
    } else {
        msg.stagger = plw->field_0x312;
    }
    msg.field_0x5C4 = plw->field_0x5C4;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x37A = plw->field_0x37A;
    msg.field_0x56B = plw->field_0x56B;
    msg.field_0x64F = plw->field_0x64F;
    msg.field_0x001 = plw->field_0x001;
    msg.gauge = (f32)plw->skill_point_0x3B8;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x3D8 = plw->field_0x3D8;
    msg.field_0x3DC = plw->field_0x3DC;
    msg.field_0x37E = plw->field_0x37E;
    msg.field_0x452 = (s8)plw->field_0x452;
    msg.field_0x567 = plw->field_0x567;
    msg.deco_skill[0] = (u8)plw->deco_skill_id[0];
    msg.deco_skill[1] = (u8)plw->deco_skill_id[1];
    msg.deco_skill[2] = (u8)plw->deco_skill_id[2];
    msg.deco_skill[3] = (u8)plw->deco_skill_id[3];
    broadcastSessionCommand(&msg, 0x4C);
}

/* Constructs the 0x4C-byte player-state message's position sub-object and returns the message. */
NetPlStateMsg::NetPlStateMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received player-state message to the addressed slot's player work: its position target, act,
 * health pair and the armed-weapon bytes, then re-enters the act the message names. */
void Pl_net_recv_state(u8 slot, const NetPlStateMsg* msg) {
    PlMoveWork* base = (PlMoveWork*)get_move_work_adrs(2);

    if (base != NULL) {
        _PLW* plw = &base[slot].pl;

        plw->kind_0x015 = msg->kind_0x15;
        plw->area_0x16 = msg->area_0x16;
        plw->field_0x3D8 = msg->field_0x3D8;
        plw->field_0x3DC = msg->field_0x3DC;
        plw->field_0x018 = msg->act_mode;
        plw->target_pos_0x090 = msg->pos;
        if ((msg->field_0x04 & 4) != 0 || msg->act_kind == 4) {
            pl_pos_blend_start(plw, 0);
        } else {
            pl_pos_blend_start(plw, 10);
        }
        if ((msg->field_0x04 & 8) != 0) {
            plw->param_0x54 = msg->field_0x14;
            plw->field_0x058 = msg->field_0x18;
            plw->field_0x0AC = plw->param_0x54;
            plw->field_0x0A8 = plw->field_0x058;
        } else {
            plw->field_0x0AC = msg->field_0x14;
            plw->field_0x0A8 = msg->field_0x18;
        }
        u16 act_no = msg->act_no;
        u8 act_kind = msg->act_kind;
        u16 mask = msg->field_0x04 & 0x2080;

        switch (act_kind) {
        case 0:
            switch (act_no) {
            case 0x0:
                Pl_act_set_motion_slot(plw, 0, 4, mask);
                break;
            case 0x6A:
                Pl_act_set_motion_slot(plw, 0, 4, mask);
                break;
            case 0x1F:
                Pl_act_set_motion_slot(plw, 1, 4, mask);
                break;
            case 0x16:
                Pl_act_set_motion_slot(plw, 3, 4, mask);
                break;
            default:
                pl_act_enter_raw(plw, act_kind, act_no, mask);
                break;
            }
            break;
        case 10:
            if (act_no == 0) {
                Pl_act_set_motion_slot(plw, 0, 4, mask);
            } else {
                pl_act_enter_raw(plw, act_kind, act_no, mask);
            }
        default:
            pl_act_enter_raw(plw, act_kind, act_no, mask);
            break;
        }
        plw->field_0x306 = msg->field_0x306;
        plw->sub_area_0x3B6 = msg->sub_area;
        plw->held_item_kind_0x26C = msg->held_item_kind;
        plw->field_0x0B6 = msg->field_0xB6;
        plw->field_0x567 = msg->field_0x567;
        if ((s32)plw->field_0x002 == 7) {
            plw->field_0x386 = msg->stagger;
        } else {
            plw->field_0x312 = msg->stagger;
        }
        plw->field_0x5C4 = msg->field_0x5C4;
        plw->health = msg->health;
        plw->health_max = msg->health_max;
        plw->field_0x37A = msg->field_0x37A;
        plw->field_0x56B = msg->field_0x56B;
        plw->field_0x64F = msg->field_0x64F;
        plw->field_0x001 = msg->field_0x001;
        plw->skill_point_0x3B8 = (u16)msg->gauge;
        plw->field_0x37E = msg->field_0x37E;
        plw->field_0x452 = (s8)msg->field_0x452;
        plw->deco_skill_id[0] = msg->deco_skill[0];
        plw->deco_skill_id[1] = msg->deco_skill[1];
        plw->deco_skill_id[2] = msg->deco_skill[2];
        plw->deco_skill_id[3] = msg->deco_skill[3];
        if (plw->field_0x00A == 0) {
            switch (plw->act_no) {
            case 0x39:
            case 0x3A:
                pl_act_net_hook_main(plw);
                break;
            case 0x99:
            case 0x9C:
            case 0x29:
                pl_act_net_hook_a(plw);
                break;
            case 0x50:
            case 0x3C:
                pl_act_net_hook_b(plw);
                break;
            case 0x7E:
            case 0x30:
                pl_act_net_hook_c(plw);
                break;
            case 0x8A:
                pl_act_net_hook_d(plw);
                break;
            }
        }
    }
}

/* Builds and sends the 0x30-byte player message from the caller's position record and the player work's
 * health pair, act number and two parameter words. */
void Pl_net_send_pos(_PLW* plw, u8 from, u8 to, u8 kind) {
    NetPlPosMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    copyVec3(&msg.pos, &plw->vec_0x03C);
    msg.field_0x10 = plw->field_0x0AC;
    msg.field_0x14 = plw->field_0x0A8;
    msg.field_0x18 = 0;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x3D8 = plw->field_0x3D8;
    msg.field_0x3DC = plw->field_0x3DC;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x37A = plw->field_0x37A;
    msg.field_0x37E = plw->field_0x37E;
    broadcastSessionCommand(&msg, 0x30);
}

/* Constructs the 0x30-byte player message's position sub-object and returns the message. */
NetPlPosMsg::NetPlPosMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received position message to the addressed slot's player work: the position target, the
 * stored parameter pair, the actor kind and area and the health pair. */
void Pl_net_recv_pos(u8 slot, const NetPlPosMsg* msg) {
    PlMoveWork* base = (PlMoveWork*)get_move_work_adrs(2);

    if (base != NULL) {
        _PLW* plw = &base[slot].pl;

        copyVec3(&plw->target_pos_0x090, &msg->pos);
        plw->field_0x0AC = msg->field_0x10;
        plw->field_0x0A8 = msg->field_0x14;
        pl_pos_blend_start(plw, 0x14);
        plw->kind_0x015 = msg->kind_0x15;
        plw->area_0x16 = msg->area_0x16;
        plw->field_0x3D8 = msg->field_0x3D8;
        plw->field_0x3DC = msg->field_0x3DC;
        plw->health = msg->health;
        plw->health_max = msg->health_max;
        plw->field_0x37A = msg->field_0x37A;
        plw->field_0x37E = msg->field_0x37E;
    }
}

/* Builds and sends the 0x28-byte attack message for the local player, but only when the network session
 * is up, this client may send, and the addressed move work really belongs to the local slot. */
void Pl_net_send_hit(u8 attack_kind, const VEC3* pos, u16 param, u8 kind) {
    NetPlAtkMsg msg;
    u8 my;
    u8 next;
    PlMoveWork* work;

    memset(&msg, 0, sizeof(msg));
    if (isServerSelectState() == 0) {
        return;
    }
    if (Pl_net_can_send() == 0) {
        return;
    }
    my = (u8)my_player_no();
    next = my + 1;
    work = (PlMoveWork*)get_move_work_adrs(2);
    if (work == NULL) {
        return;
    }
    work = &work[my];
    if (work->pl.chunk_ofs != my) {
        return;
    }
    msg.hdr.fill(my, next, kind);
    copyVec3(&msg.pos, pos);
    msg.rot_0x10.x = 0;
    msg.rot_0x10.y = param;
    msg.rot_0x10.z = 0;
    msg.kind_0x15 = work->pl.kind_0x015;
    msg.area_0x16 = attack_kind;
    msg.field_0x01F = pl_act_stage_get(&work->pl);
    msg.field_0x416 = work->pl.field_0x416;
    msg.field_0x41C = work->pl.field_0x41C;
    msg.field_0x5C4 = work->pl.field_0x5C4;
    msg.field_0x009 = work->pl.kind_0x09;
    msg.field_0x001 = work->pl.field_0x001;
    work->pl.field_0x645 = 1;
    broadcastSessionCommand(&msg, 0x28);
}

/* Constructs the 0x28-byte attack message's position sub-object and returns the message. */
NetPlAtkMsg::NetPlAtkMsg() {
    VEC3_ctor(&pos);
}

/* Sends the attack message with the caller's attack kind and position, tagged as kind 3. */
void Pl_net_send_hit_kind3(u8 attack_kind, const VEC3* pos, u16 param) {
    Pl_net_send_hit(attack_kind, pos, param, 3);
}

/* Sends the attack message with the caller's attack kind and position, tagged as kind 5. */
void Pl_net_send_hit_kind5(u8 attack_kind, const VEC3* pos, u16 param) {
    Pl_net_send_hit(attack_kind, pos, param, 5);
}

/* Applies a received attack message to the addressed slot's player work: the position target and current
 * position, the rotation, the hit parameters and the actor kind and area, then spawns the hit effect. */
void Pl_net_recv_hit(u8 slot, const NetPlAtkMsg* msg, u32 extra) {
    PlMoveWork* base = (PlMoveWork*)get_move_work_adrs(2);

    if (base != NULL) {
        _PLW* plw = &base[slot].pl;

        copyVec3(&plw->target_pos_0x090, &msg->pos);
        copyVec3(&plw->vec_0x03C, &msg->pos);
        eft_rot_vec_copy(&plw->rot_0x54, const_cast<_CP_VECTOR*>(&msg->rot_0x10));
        plw->kind_0x015 = msg->kind_0x15;
        plw->area_0x16 = msg->area_0x16;
        pl_pos_blend_start(plw, 0);
        plw->field_0x416 = msg->field_0x416;
        plw->field_0x41C = msg->field_0x41C;
        plw->field_0x5C4 = msg->field_0x5C4;
        plw->kind_0x09 = msg->field_0x009;
        plw->field_0x001 = msg->field_0x001;
        plw->field_0x01F = (u8)msg->field_0x01F;
        pl_hit_effect_spawn(plw, &plw->vec_0x03C, (u16)plw->field_0x058, plw->kind_0x015, plw->area_0x16, extra);
    }
}

/* Builds and sends the 0x38-byte player message: the position, the running act, the health pair and the
 * armed weapon's id/timer pair. */
void Pl_net_send_extra(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlExtraMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    msg.act_mode = plw->field_0x018;
    msg.field_0x04 = param;
    msg.pos = plw->vec_0x03C;
    msg.field_0x14 = plw->field_0x0AC;
    msg.field_0x18 = plw->field_0x0A8;
    msg.act_kind = plw->field_0x00A;
    msg.act_no = plw->act_no;
    msg.field_0xB6 = plw->field_0x0B6;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x001 = plw->field_0x001;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x37E = plw->field_0x37E;
    msg.field_0x650 = plw->field_0x650;
    msg.field_0x652 = plw->field_0x652;
    msg.field_0x655 = plw->field_0x655;
    broadcastSessionCommand(&msg, 0x38);
}

/* Constructs the 0x38-byte player message's position sub-object and returns the message. */
NetPlExtraMsg::NetPlExtraMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received extra-state message to the addressed slot's player work: the position target, the
 * parameter pair, the health pair and the armed weapon's id/timer pair, then re-enters the act it names. */
void Pl_net_recv_extra(u8 slot, const NetPlExtraMsg* msg) {
    PlMoveWork* base = (PlMoveWork*)get_move_work_adrs(2);

    if (base != NULL) {
        _PLW* plw = &base[slot].pl;

        plw->kind_0x015 = msg->kind_0x15;
        plw->area_0x16 = msg->area_0x16;
        plw->target_pos_0x090 = msg->pos;
        if ((msg->field_0x04 & 4) != 0 || msg->act_kind == 4) {
            pl_pos_blend_start(plw, 0);
        } else {
            pl_pos_blend_start(plw, 10);
        }
        if ((msg->field_0x04 & 8) != 0) {
            plw->param_0x54 = msg->field_0x14;
            plw->field_0x058 = msg->field_0x18;
            plw->field_0x0AC = plw->param_0x54;
            plw->field_0x0A8 = plw->field_0x058;
        } else {
            plw->field_0x0AC = msg->field_0x14;
            plw->field_0x0A8 = msg->field_0x18;
        }
        pl_act_enter_raw(plw, msg->act_kind, msg->act_no, 0);
        plw->field_0x0B6 = msg->field_0xB6;
        plw->health = msg->health;
        plw->health_max = msg->health_max;
        plw->field_0x001 = msg->field_0x001;
        plw->field_0x37E = msg->field_0x37E;
        plw->field_0x650 = msg->field_0x650;
        plw->field_0x652 = msg->field_0x652;
        plw->field_0x655 = msg->field_0x655;
    }
}

/* Builds and sends the 0x0C-byte short player message: the primary act (kind 7) carries the act's
 * follow-up byte and the caller's value, every other act the follow-up byte, the weapon-class byte and
 * the armed weapon's id/timer pair. */
void Pl_net_send_item(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlShortMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    if (kind == 7) {
        msg.field_0x06 = plw->chunk_ofs;
        msg.field_0x07 = param;
    } else {
        msg.field_0x04 = param;
        msg.field_0x06 = plw->field_0x656;
        msg.field_0x07 = plw->chunk_ofs;
    }
    msg.field_0x650 = plw->field_0x650;
    msg.field_0x652 = plw->field_0x652;
    broadcastSessionCommand(&msg, 0xC);
}

/* Handles a received item-use request for the local player: when the act, the armed weapon and the item
 * timer agree with the request it spends the item and answers with a grant, otherwise with a refusal. */
void Pl_net_recv_item_use(u8 slot, const NetPlShortMsg* msg) {
    PlMoveWork* base;
    _PLW* plw;

    if (msg->field_0x07 == (s8)my_player_no() && (base = (PlMoveWork*)get_move_work_adrs(2), base != NULL)) {
        plw = &base[(s8)my_player_no()].pl;
        if ((Pl_act_ck(plw, 0, 0x14) != 0 || Pl_act_ck(plw, 0, 0x9E) != 0) &&
            (plw->field_0x655 == msg->field_0x06 || plw->field_0x655 == 0xFF) &&
            plw->field_0x650 == msg->field_0x650 && plw->field_0x652 == msg->field_0x652 &&
            Pl_item_timer_get(plw, plw->field_0x650) >= plw->field_0x652) {
            plw->field_0x656 = msg->field_0x06;
            pl_item_add(plw, plw->field_0x650, -plw->field_0x652);
            Pl_net_send(plw, 8, 1);
            plw->field_0x656 = 0xFF;
            return;
        }
        Pl_net_send(plw, 8, 2);
    }
}

/* Handles a received item-use answer for the local player: a grant adds the item and plays the use
 * state, a refusal only records the refused flag. */
void Pl_net_recv_item_result(u8 slot, const NetPlShortMsg* msg) {
    PlMoveWork* base;
    _PLW* plw;

    if (msg->field_0x06 == (s8)my_player_no() && (base = (PlMoveWork*)get_move_work_adrs(2), base != NULL)) {
        plw = &base[(s8)my_player_no()].pl;
        if ((msg->field_0x04 & 1) != 0) {
            plw->field_0x656 = 1;
            pl_item_add(plw, msg->field_0x650, msg->field_0x652);
            if (item_se_ck(msg->field_0x650) == 1) {
                se_slot_req(4);
            }
            pl_model_state_set(plw, 2, 0x1D, msg->field_0x650);
        } else {
            plw->field_0x656 = 2;
        }
    }
}

/* Builds and sends the 0x4C-byte alternate player-state message: the same head as the state message, with
 * the shell angle and the armed-weapon bytes in place of its sub-area and arming fields. */
void Pl_net_send_state_alt(_PLW* plw, u8 from, u8 to, u8 kind, u16 param) {
    NetPlStateAltMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    msg.act_mode = plw->field_0x018;
    msg.field_0x04 = param;
    msg.pos = plw->vec_0x03C;
    msg.field_0x14 = plw->field_0x0AC;
    msg.field_0x18 = plw->field_0x0A8;
    msg.act_kind = plw->field_0x00A;
    msg.act_no = plw->act_no;
    msg.field_0x306 = plw->field_0x306;
    msg.held_item_kind = plw->held_item_kind_0x26C;
    msg.field_0xB6 = plw->field_0x0B6;
    msg.health = plw->health;
    msg.health_max = plw->health_max;
    msg.field_0x37A = plw->field_0x37A;
    msg.shell_angle = plw->shell_ang_0x583;
    msg.field_0x64F = plw->field_0x64F;
    msg.field_0x001 = plw->field_0x001;
    msg.gauge = (f32)plw->skill_point_0x3B8;
    msg.kind_0x15 = plw->kind_0x015;
    msg.area_0x16 = plw->area_0x16;
    msg.field_0x3D8 = plw->field_0x3D8;
    msg.field_0x3DC = plw->field_0x3DC;
    msg.field_0x37E = plw->field_0x37E;
    msg.deco_skill[0] = (u8)plw->deco_skill_id[0];
    msg.deco_skill[1] = (u8)plw->deco_skill_id[1];
    msg.deco_skill[2] = (u8)plw->deco_skill_id[2];
    msg.deco_skill[3] = (u8)plw->deco_skill_id[3];
    broadcastSessionCommand(&msg, 0x4C);
}

/* Constructs the 0x4C-byte player-state message's position sub-object and returns the message. */
NetPlStateAltMsg::NetPlStateAltMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received alternate player-state message to the addressed slot's player work, then re-enters the
 * act it names. */
void Pl_net_recv_state_alt(u8 slot, const NetPlStateAltMsg* msg) {
    PlMoveWork* base = (PlMoveWork*)get_move_work_adrs(2);

    if (base != NULL) {
        _PLW* plw = &base[slot].pl;

        plw->kind_0x015 = msg->kind_0x15;
        plw->area_0x16 = msg->area_0x16;
        plw->field_0x3D8 = msg->field_0x3D8;
        plw->field_0x3DC = msg->field_0x3DC;
        plw->field_0x018 = msg->act_mode;
        plw->target_pos_0x090 = msg->pos;
        if ((msg->field_0x04 & 4) != 0) {
            pl_pos_blend_start(plw, 0);
        } else {
            pl_pos_blend_start(plw, 10);
        }
        if ((msg->field_0x04 & 8) != 0) {
            plw->param_0x54 = msg->field_0x14;
            plw->field_0x058 = msg->field_0x18;
            plw->field_0x0AC = plw->param_0x54;
            plw->field_0x0A8 = plw->field_0x058;
        } else {
            plw->field_0x0AC = msg->field_0x14;
            plw->field_0x0A8 = msg->field_0x18;
        }
        pl_act_enter_raw(plw, msg->act_kind, msg->act_no, 0);
        plw->field_0x0B6 = msg->field_0xB6;
        plw->health = msg->health;
        plw->health_max = msg->health_max;
        plw->field_0x37A = msg->field_0x37A;
        plw->shell_ang_0x583 = msg->shell_angle;
        plw->field_0x64F = msg->field_0x64F;
        plw->field_0x306 = msg->field_0x306;
        plw->skill_point_0x3B8 = (u16)msg->gauge;
        plw->field_0x001 = msg->field_0x001;
        plw->held_item_kind_0x26C = msg->held_item_kind;
        plw->field_0x37E = msg->field_0x37E;
        plw->deco_skill_id[0] = msg->deco_skill[0];
        plw->deco_skill_id[1] = msg->deco_skill[1];
        plw->deco_skill_id[2] = msg->deco_skill[2];
        plw->deco_skill_id[3] = msg->deco_skill[3];
    }
}

/* The client's own sender: when the session is up, this client may send and the addressed move work
 * really is the local slot's, dispatches on the act kind to the matching message builder. */
void Pl_net_send(_PLW* plw, u8 kind, u16 param) {
    u8 my;
    u8 next;

    if (isServerSelectState() == 0) {
        return;
    }
    if (Pl_net_can_send() == 0) {
        return;
    }
    my = (u8)my_player_no();
    next = my + 1;
    if (plw->chunk_ofs != my) {
        return;
    }
    switch (kind) {
    case 1:
        Pl_net_send_state(plw, my, next, kind, param);
        break;
    case 2:
        Pl_net_send_pos(plw, my, next, kind);
        break;
    case 6:
        Pl_net_send_extra(plw, my, next, kind, param);
        break;
    case 7:
    case 8:
        Pl_net_send_item(plw, my, next, kind, param);
        break;
    case 9:
        Pl_net_send_state_alt(plw, my, next, kind, param);
        break;
    }
}

/* The client's own receiver: dispatches a received player message on its kind to the matching applier,
 * ignoring messages this client sent itself. */
void Pl_net_recv(u8 slot, const NetMsgHeader* msg) {
    s8 own;

    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (s8)my_player_no();
    if (msg->from_slot == own) {
        return;
    }
    switch (msg->kind) {
    case 1:
        Pl_net_recv_state(slot, (const NetPlStateMsg*)msg);
        break;
    case 2:
        Pl_net_recv_pos(slot, (const NetPlPosMsg*)msg);
        break;
    case 3:
        Pl_net_recv_hit(slot, (const NetPlAtkMsg*)msg, 0);
        break;
    case 5:
        Pl_net_recv_hit(slot, (const NetPlAtkMsg*)msg, 1);
        break;
    case 6:
        Pl_net_recv_extra(slot, (const NetPlExtraMsg*)msg);
        break;
    case 7:
        Pl_net_recv_item_use(slot, (const NetPlShortMsg*)msg);
        break;
    case 8:
        Pl_net_recv_item_result(slot, (const NetPlShortMsg*)msg);
        break;
    case 9:
        Pl_net_recv_state_alt(slot, (const NetPlStateAltMsg*)msg);
        break;
    }
}

/* Fills the identity record an enemy message carries: the enemy's id and phase, and the sender's step. */
void em_net_ident_set(_ENEMY_WORK* work, u8 step, NetEmIdent* ident) {
    ident->enemy_id = work->field_0x01A;
    ident->phase = work->field_0x016;
    ident->step = step;
}

/* Packs the damage level of each of the eight parts, and the break data of the first two parts that have
 * any, into the part-state block. */
void em_net_parts_pack(_ENEMY_WORK* work, NetEmParts* out) {
    u8 part;
    u8 broken = 0;
    EmPartRec* rec;

    for (part = 0; part < 8; part++) {
        rec = &work->parts_0x838[part];
        out->damage_level[part] = em_parts_damage_level_get(work, part);
        out->value[part] = rec->value_0x02;
        if (broken < 2 && em_get_part_limit(work)->break_limit_0x04 > 0) {
            out->broken_part[broken] = em_parts_state_get(work, part);
            out->broken_value[broken] = rec->value_0x04;
            broken++;
        }
    }
}

/* Applies a part-state block: raises each part to the received damage level and restores the values and the
 * break data of the first two parts that have any. */
void em_net_parts_unpack(_ENEMY_WORK* work, const NetEmParts* in) {
    u8 part;
    u8 broken = 0;
    EmPartRec* rec;

    for (part = 0; part < 8; part++) {
        rec = &work->parts_0x838[part];
        while (em_parts_damage_level_get(work, part) < in->damage_level[part]) {
            em_parts_damage_add(work, part, 1);
            em_parts_refresh(work, part, 0);
        }
        rec->damage_level = in->damage_level[part];
        rec->value_0x02 = in->value[part];
        if (broken < 2 && em_get_part_limit(work)->break_limit_0x04 > 0) {
            rec->field_0x01 = in->broken_part[broken];
            rec->value_0x04 = in->broken_value[broken];
            broken++;
        }
    }
}

/* Builds and sends the 0x6C-byte enemy-state message: position, target, rotation, the action, the special
 * part and the status flags, with the part-state block. */
void em_net_send_state(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmStateMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, work->net_seq_0x01E++, &msg.ident);
    work->sync_flag_0x468 = 0;
    copyVec3(&msg.pos, &work->pos);
    msg.height = work->field_0x1AC;
    copyVec3(&msg.target, &work->vec_0x36C);
    msg.rot_x = (u16)work->field_0x1BC;
    msg.rot_y = (u16)work->field_0x1C0;
    msg.rot_z = (u16)work->field_0x1C4;
    msg.field_0x1EE = work->field_0x1EE;
    msg.field_0x1EF = work->field_0x1EF;
    msg.field_0x1F0 = work->field_0x1F0;
    msg.field_0x1E7 = work->field_0x1E7;
    msg.field_0x1E4 = work->field_0x1E4;
    msg.field_0x380 = work->field_0x380;
    msg.state_0x381 = work->state_0x381;
    if (work->field_0x380 == 3 && work->state_0x381 == 2) {
        if (work->field_0x382 != 0xFF) {
            _ENEMY_WORK* special = &((_ENEMY_WORK*)get_move_work_adrs(3))[work->field_0x382];

            if (special->active != 0) {
                msg.special_part = special->field_0x01A;
            } else {
                msg.special_part = 0xFFFF;
            }
        } else {
            msg.special_part = 0xFFFF;
        }
    } else if (work->field_0x380 == 8) {
        msg.special_part = work->field_0x382 | (work->field_0x99A << 8);
    } else {
        msg.special_part = work->field_0x382;
    }
    msg.field_0x9F8 = work->field_0x9F8;
    msg.field_0x383 = work->field_0x383;
    msg.field_0x384 = work->field_0x384;
    msg.field_0x388 = work->field_0x388;
    msg.area_no = work->area_no;
    msg.field_0x9F7 = work->field_0x9F7;
    msg.field_0x43D = work->field_0x43D;
    msg.field_0x94F = work->field_0x94F;
    msg.status_bits = 0;
    if (work->field_0x43E == 1) {
        msg.status_bits |= 1;
    }
    if (em_alt_mode_ck(work) == 1) {
        msg.status_bits |= 2;
    }
    switch (work->field_0x89F) {
    case 1:
        msg.status_bits |= 4;
        break;
    case 2:
        msg.status_bits |= 8;
        break;
    case 3:
        msg.status_bits |= 0xC;
        break;
    }
    if (work->field_0x916 > 0) {
        msg.status_bits |= 0x10;
    }
    if (em_break_state_ck(work) == 1) {
        msg.status_bits |= 0x20;
    }
    if (em_status_ck(work, 2) == 1) {
        msg.status_bits |= 0x40;
    }
    if (work->field_0x1F9 == 1) {
        msg.status_bits |= 0x80;
    }
    if (work->field_0x1FA == 1) {
        msg.status_bits |= 0x100;
    }
    if ((work->field_0x1FC != 0 || work->field_0x1FD != 0) && work->field_0x9F8 != 0xFF) {
        u8 armed_area = work->field_0x9F7;

        if (armed_area != 0xFF && armed_area != work->area_no) {
            msg.status_bits |= 0x200;
            work->field_0x1FD = 1;
        }
    }
    if (work->field_0x99B != 0) {
        msg.status_bits |= 0x400;
    }
    if (work->field_0x439 == 0) {
        msg.status_bits |= 0x800;
    }
    msg.field_0x91C = work->field_0x91C;
    msg.field_0x919 = work->field_0x919;
    msg.field_0x8A2 = work->field_0x8A2;
    msg.field_0x961 = work->stack_0x961[0];
    msg.field_0x7A0 = work->field_0x7A0;
    em_net_parts_pack(work, &msg.parts);
    broadcastSessionCommand(&msg, 0x6C);
}

/* Constructs the 0x6C-byte enemy-state message's two position sub-objects and returns the message. */
NetEmStateMsg::NetEmStateMsg() {
    VEC3_ctor(&pos);
    VEC3_ctor(&target);
}

/* Applies a received enemy-state message: position, target, rotation, the special part and the status
 * flags, raising the status levels and part damage to the received values. */
void em_net_recv_state(_ENEMY_WORK* work, NetEmStateMsg* msg) {
    u8 mode;
    u8 kind;
    u32 alt_mode;

    work->sync_flag_0x468 = 0;
    if ((u32)em_act_ck(work, 12, 0xFF) == 1) {
        work->field_0x016 += 1;
        work->field_0x012 = 0;
        em_act_advance(work, 1);
    }
    copyVec3(&work->pos, &msg->pos);
    work->field_0x1AC = msg->height;
    work->field_0x1BC = msg->rot_x;
    work->field_0x1C0 = msg->rot_y;
    work->field_0x1C4 = msg->rot_z;
    work->bits_0x1EC = msg->field_0x1F0;
    work->field_0x1E7 = msg->field_0x1E7;
    work->field_0x1E4 = msg->field_0x1E4;
    if (work->area_no != msg->area_no) {
        work->field_0x9F7 = msg->area_no;
        em_area_change(work, 0);
    }
    work->field_0x9F7 = msg->field_0x9F7;
    copyVec3(&work->vec_0x36C, &msg->target);
    kind = msg->field_0x380;
    work->field_0x380 = kind;
    mode = msg->state_0x381;
    work->state_0x381 = mode;
    if (kind == 3 && mode == 2) {
        if (msg->special_part != 0xFFFF) {
            _ENEMY_WORK* special;

            if ((u8)em_get_unique_work(msg->special_part, &special, NULL) != 1) {
                work->field_0x382 = 0xFF;
            } else {
                work->field_0x382 = special->group;
            }
        } else {
            work->field_0x382 = 0xFF;
        }
    } else if (kind == 8) {
        work->field_0x382 = (u8)msg->special_part;
        work->field_0x99A = (u8)(msg->special_part >> 8);
        em_special_part_apply(work, work->state_0x381);
    } else {
        work->field_0x382 = (u8)msg->special_part;
    }
    work->field_0x9F8 = msg->field_0x9F8;
    work->field_0x383 = msg->field_0x383;
    work->field_0x384 = msg->field_0x384;
    work->field_0x388 = msg->field_0x388;
    em_status_set(work, msg->field_0x43D);
    work->field_0x94F = msg->field_0x94F;
    if ((msg->status_bits & 1) != 0) {
        work->field_0x43E = 1;
    } else {
        work->field_0x43E = 0;
    }
    if ((msg->status_bits & 2) != 0) {
        em_alt_mode_set(work, 1);
    } else {
        em_alt_mode_set(work, 0);
    }
    switch (msg->status_bits & 0xC) {
    case 4:
        alt_mode = 1;
        break;
    case 8:
        alt_mode = 2;
        break;
    case 12:
        alt_mode = 3;
        break;
    default:
        alt_mode = 0;
        break;
    }
    if (work->field_0x89F != alt_mode) {
        em_motion_mode_set(work, alt_mode);
    }
    if ((msg->status_bits & 0x10) != 0) {
        if (work->field_0x916 <= 0) {
            em_level_raise(work, 0);
        }
        while (work->field_0x919 < msg->field_0x919) {
            em_level_raise(work, 0);
        }
    } else {
        work->field_0x919 = msg->field_0x919;
        work->field_0x916 = 0;
    }
    if ((msg->status_bits & 0x20) != 0) {
        if (em_break_state_ck(work) == 0) {
            em_net_flag_apply_3(work);
        }
    } else {
        work->field_0x94C = 0;
    }
    if ((msg->status_bits & 0x80) != 0) {
        work->field_0x1F9 = 1;
    } else {
        work->field_0x1F9 = 0;
    }
    if ((msg->status_bits & 0x100) != 0) {
        work->field_0x1FA = 1;
    } else {
        work->field_0x1FA = 0;
    }
    if ((msg->status_bits & 0x800) != 0) {
        work->field_0x439 = 0;
    }
    if ((msg->status_bits & 0x200) != 0) {
        work->field_0x1FD = 1;
    } else {
        work->field_0x1FD = 0;
    }
    if ((msg->status_bits & 0x400) != 0) {
        work->field_0x99B = 1;
    } else {
        work->field_0x99B = 0;
    }
    if (work->field_0x91C < msg->field_0x91C) {
        while (work->field_0x91C < msg->field_0x91C) {
            em_status_advance(work);
        }
    }
    if ((msg->status_bits & 0x40) != 0) {
        if (em_status_ck(work, 2) == 0) {
            em_status_start(work);
        }
    } else if (em_status_ck(work, 2) == 1) {
        em_status_advance(work);
    }
    work->field_0x8A2 = msg->field_0x8A2;
    work->stack_0x961[0] = msg->field_0x961;
    work->field_0x7A0 = msg->field_0x7A0;
    em_net_parts_unpack(work, &msg->parts);
    em_act_step_arm(work, msg->field_0x1EE, msg->field_0x1EF, 4);
    em_state_refresh(work);
}

/* Builds and sends the 0x60-byte enemy-target message: position, rotation, the bound player, the status
 * counters and the part-state block; `param` of 1 (or a ready count of one) marks it confirmed. */
void em_net_send_target(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmTargetMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, work->net_seq_0x01E++, &msg.ident);
    work->sync_flag_0x468 = 0;
    copyVec3(&msg.pos, &work->pos);
    msg.height = work->field_0x1AC;
    msg.rot_x = (u16)work->field_0x1BC;
    msg.rot_y = (u16)work->field_0x1C0;
    msg.rot_z = (u16)work->field_0x1C4;
    msg.owner_slot = work->owner_slot_0x1F6;
    msg.area_no = work->area_no;
    msg.field_0x94F = work->field_0x94F;
    msg.field_0x934 = work->field_0x934;
    msg.field_0x92E = work->field_0x92E;
    msg.field_0x924 = work->field_0x924;
    msg.field_0x93A = work->field_0x93A;
    msg.field_0x93E = work->field_0x93E;
    msg.field_0x944 = work->field_0x944;
    msg.field_0x926 = work->field_0x926;
    msg.field_0x916 = work->field_0x916;
    msg.field_0x91C = work->field_0x91C;
    msg.field_0x919 = work->field_0x919;
    msg.field_0x439 = work->field_0x439;
    msg.field_0x1F2 = work->field_0x1F2;
    msg.field_0x8A6 = work->field_0x8A6;
    msg.field_0x961 = work->stack_0x961[0];
    msg.field_0x7A0 = work->field_0x7A0;
    em_net_parts_pack(work, &msg.parts);
    if (param == 1 || isReadyCountOne() == 1) {
        msg.confirmed = 1;
    } else {
        msg.confirmed = 0;
    }
    broadcastSessionCommand(&msg, 0x60);
}

/* Constructs the 0x60-byte enemy-target message's position sub-object and returns the message. */
NetEmTargetMsg::NetEmTargetMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received enemy-target message: when it is confirmed (or the ready count is one) it moves the
 * enemy's position by the change in its world position, then copies the rotation, the counters and the
 * part state and adopts the bound player. */
void em_net_recv_target(_ENEMY_WORK* work, const NetEmTargetMsg* msg) {
    NetEmTargetMsg unused_msg;
    VEC3 cur_pos;
    VEC3 new_pos;

    VEC3_ctor(&cur_pos);
    VEC3_ctor(&new_pos);
    work->sync_flag_0x468 = 0;
    if (msg->confirmed != 0 || isReadyCountOne() == 1) {
        if (work->area_no != msg->area_no && work->field_0x9F7 == msg->area_no) {
            VEC3 delta;

            copyVec3(&cur_pos, &get_worldworld_pos(&work->pos, work->area_no));
            copyVec3(&new_pos, &get_worldworld_pos(const_cast<VEC3*>(&msg->pos), msg->area_no));
            subVec3(&delta, &new_pos, &cur_pos);
            addVec3To(&work->pos_offset_0x470, &delta);
            copyVec3(&work->pos, &msg->pos);
            work->field_0x1AC = msg->height;
            work->field_0x1BC = msg->rot_x;
            work->field_0x1C0 = msg->rot_y;
            work->field_0x1C4 = msg->rot_z;
            em_area_change(work, 0);
        }
        work->field_0x94F = msg->field_0x94F;
        work->field_0x934 = msg->field_0x934;
        work->field_0x92E = msg->field_0x92E;
        work->field_0x924 = msg->field_0x924;
        work->field_0x93A = msg->field_0x93A;
        work->field_0x93E = msg->field_0x93E;
        work->field_0x944 = msg->field_0x944;
        work->field_0x8A6 = msg->field_0x8A6;
        if (work->field_0x91C < msg->field_0x91C) {
            while (work->field_0x91C < msg->field_0x91C) {
                em_status_advance(work);
            }
        }
        if (msg->field_0x926 > 0) {
            if (em_status_ck(work, 2) == 0) {
                em_status_start(work);
            } else {
                work->field_0x926 = msg->field_0x926;
            }
        } else if (em_status_ck(work, 2) == 1) {
            em_status_advance(work);
        } else {
            work->field_0x926 = 0;
        }
        work->field_0x919 = msg->field_0x919;
        work->field_0x439 = msg->field_0x439;
        work->field_0x1F2 = msg->field_0x1F2;
        work->stack_0x961[0] = msg->field_0x961;
        work->field_0x7A0 = msg->field_0x7A0;
        em_net_parts_unpack(work, &msg->parts);
        work->owner_slot_0x1F6 = msg->owner_slot;
        if (work->owner_slot_0x1F6 == (s8)my_player_no()) {
            work->field_0x1F5 = 1;
        } else {
            work->field_0x1F5 = 0;
        }
        if (isReadyCountOne() == 1) {
            em_net_send(work, 2, 1);
        }
    }
}

/* Builds and sends the 0x0A-byte enemy action-flag message carrying the enemy's pending action flags. */
void em_net_send_flags(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmFlagsMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, 0, &msg.ident);
    msg.flags = work->field_0xAEE;
    broadcastSessionCommand(&msg, 0xA);
}

/* Replays each action flag a received message carries on the enemy, then clears its pending flags. */
void em_net_recv_flags(_ENEMY_WORK* work, const NetEmFlagsMsg* msg) {
    if ((msg->flags & 1) != 0) {
        em_net_flag_apply_0(work);
    }
    if ((msg->flags & 2) != 0) {
        em_net_flag_apply_1(work);
    }
    if ((msg->flags & 4) != 0) {
        em_net_flag_apply_2(work);
    }
    if ((msg->flags & 0x10) != 0) {
        em_net_flag_apply_4(work);
    }
    if ((msg->flags & 0x20) != 0) {
        em_net_flag_apply_5(work);
    }
    if ((msg->flags & 0x80) != 0) {
        em_net_flag_apply_7(work);
    }
    if ((msg->flags & 8) != 0) {
        em_net_flag_apply_3(work);
    }
    if ((msg->flags & 0x40) != 0) {
        em_net_flag_apply_6(work);
    }
    work->field_0xAEE = 0;
}

/* Builds and sends the 0x10-byte enemy action message: the caller's sub-kind, the enemy's action id and
 * its scale. */
void em_net_send_act(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmActMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, 0, &msg.ident);
    msg.sub_kind = param;
    msg.act_id = work->field_0x7C8;
    msg.scale = work->field_0x1D0;
    broadcastSessionCommand(&msg, 0x10);
}

/* Applies a received enemy action message: sub-kind 0 advances the phase and arms the received action,
 * sub-kind 1 clears the running action. */
void em_net_recv_act(_ENEMY_WORK* work, const NetEmActMsg* msg) {
    switch (msg->sub_kind) {
    case 0:
        work->field_0x016 += 1;
        work->field_0x012 = 0;
        work->field_0x7C8 = msg->act_id;
        work->field_0x1D0 = msg->scale;
        em_act_advance(work, 1);
        break;
    case 1:
        em_act_end(work, 0);
        break;
    }
}

/* Builds and sends the 0x0C-byte enemy count message: the caller's sub-kind and, for sub-kinds 1 and 2,
 * the enemy's level counter or its signed +0x014 byte. */
void em_net_send_count(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmCountMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, work->net_seq_0x01E++, &msg.ident);
    work->sync_flag_0x468 = 0;
    msg.sub_kind = param;
    switch (param) {
    case 1:
        msg.value = work->field_0x919;
        break;
    case 2:
        msg.value = (s8)work->field_0x014;
        break;
    default:
        msg.value = 0;
        break;
    }
    broadcastSessionCommand(&msg, 0xC);
}

/* Applies a received enemy count message: sub-kind 1 raises the level counter once, sub-kind 2 breaks
 * every part whose bit is set. */
void em_net_recv_count(_ENEMY_WORK* work, const NetEmCountMsg* msg) {
    work->sync_flag_0x468 = 0;
    switch (msg->sub_kind) {
    case 1:
        if (work->field_0x919 < msg->value) {
            em_level_raise(work, 0);
        }
        break;
    case 2: {
        s32 part;

        for (part = 0; part < 8; part++) {
            if ((msg->value & (1 << part)) != 0) {
                em_parts_break(work, part);
            }
        }
        break;
    }
    }
}

/* Builds and sends the 0x0A-byte enemy bind message carrying the caller's player slot. */
void em_net_send_bind(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmBindMsg msg;

    memset(&msg, 0, sizeof(msg));
    work->sync_flag_0x468 = 0;
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, work->net_seq_0x01E++, &msg.ident);
    msg.slot = param;
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received enemy bind message: the enemy is bound to the named player slot, and the local client
 * re-sends its state when the slot is its own. */
void em_net_recv_bind(_ENEMY_WORK* work, NetEmBindMsg* msg) {
    work->sync_flag_0x468 = 0;
    if (msg->slot == (s8)my_player_no()) {
        work->field_0x1F5 = 0;
        work->owner_slot_0x1F6 = (s8)msg->slot;
        em_net_send(work, 2, 0);
    } else {
        work->field_0x1F5 = 0;
        work->owner_slot_0x1F6 = (s8)msg->slot;
    }
}

/* Builds and sends the 0x0A-byte enemy release message: the caller's flag and the enemy's bound slot. */
void em_net_send_release(_ENEMY_WORK* work, u8 from, s32 to, u8 kind, u8 param) {
    NetEmReleaseMsg msg;

    memset(&msg, 0, sizeof(msg));
    work->sync_flag_0x468 = 0;
    msg.hdr.fill(from, to, kind);
    em_net_ident_set(work, 0, &msg.ident);
    msg.flag = param;
    msg.slot = work->owner_slot_0x1F6;
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received enemy release message: flag 0 re-sends the state when the ready count is one, otherwise
 * the enemy is bound to the named slot, and re-sent when that slot is the local client's and the enemy is free. */
void em_net_recv_release(_ENEMY_WORK* work, NetEmReleaseMsg* msg) {
    work->sync_flag_0x468 = 0;
    if (msg->flag == 0) {
        if (isReadyCountOne() == 1) {
            em_net_send(work, 7, 1);
        }
    } else if (msg->slot == (s8)my_player_no()) {
        if (em_busy_ck(work) == 0) {
            work->owner_slot_0x1F6 = (s8)msg->slot;
            em_net_send(work, 2, 0);
        }
    } else {
        work->field_0x1F5 = 0;
        work->owner_slot_0x1F6 = (s8)msg->slot;
    }
}

/* The client's own enemy sender: when the session is up, this client may send and the enemy carries an
 * id, dispatches on the enemy act kind to the matching message builder. */
void em_net_send(_ENEMY_WORK* work, u8 kind, u16 param) {
    u8 own;

    if (isServerSelectState() == 0) {
        return;
    }
    if (work->field_0x01A == 0xFFFF) {
        return;
    }
    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (u8)my_player_no();
    switch (kind) {
    case 1:
        em_net_send_state(work, own, 5, kind, param);
        break;
    case 2:
        em_net_send_target(work, own, 5, kind, param);
        break;
    case 3:
        em_net_send_flags(work, own, 5, kind, param);
        break;
    case 4:
        em_net_send_act(work, own, 5, kind, param);
        break;
    case 5:
        em_net_send_count(work, own, 5, kind, param);
        break;
    case 6:
        em_net_send_bind(work, own, 5, kind, param);
        break;
    case 7:
        em_net_send_release(work, own, 5, kind, param);
        break;
    }
}

/* The client's own enemy receiver: resolves the message's enemy id and dispatches on its kind to the
 * matching applier, ignoring messages this client sent itself. */
void em_net_recv(u8 slot, const NetEmStateMsg* msg) {
    _ENEMY_WORK* work;
    s8 own;

    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (s8)my_player_no();
    if (msg->hdr.from_slot == own) {
        return;
    }
    if ((u8)em_get_unique_work(msg->ident.enemy_id, &work, NULL) != 1) {
        return;
    }
    switch (msg->hdr.kind) {
    case 1:
        em_net_recv_state(work, (NetEmStateMsg*)msg);
        break;
    case 2:
        em_net_recv_target(work, (const NetEmTargetMsg*)msg);
        break;
    case 3:
        em_net_recv_flags(work, (const NetEmFlagsMsg*)msg);
        break;
    case 4:
        em_net_recv_act(work, (const NetEmActMsg*)msg);
        break;
    case 5:
        em_net_recv_count(work, (const NetEmCountMsg*)msg);
        break;
    case 6:
        em_net_recv_bind(work, (NetEmBindMsg*)msg);
        break;
    case 7:
        em_net_recv_release(work, (NetEmReleaseMsg*)msg);
        break;
    }
}

/* Builds and sends the 0x0C-byte enemy-control status message carrying the last event the control work
 * recorded. */
void emc_net_send_status(EmcWork* emc, u8 from, s32 to, u8 kind) {
    NetEmcStatusMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    msg.status.assign(&emc->status_0xD9C);
    broadcastSessionCommand(&msg, 0xC);
}

/* Copies the 8-byte per-enemy status record the enemy sender reads out of an enemy work record. */
void EmcStatus::assign(const EmcStatus* src) {
    enemy_id = src->enemy_id;
    phase = src->phase;
    step = src->step;
    kind = src->kind;
    value_0x05 = src->value_0x05;
    value_0x06 = src->value_0x06;
    value_0x07 = src->value_0x07;
}

/* Replays a received enemy-control event: on the enemy's work record (found) it advances the phase, step
 * and action state to the received ones; on a mini enemy it raises the matching phase and step; with no
 * enemy yet it forwards a spawn request for event kinds 1 and 5. */
void emc_net_recv_status(NetEmcStatusMsg* msg) {
    _ENEMY_WORK* work;
    _ENEMY_MINI_WORK* mini;
    u8 found;

    found = em_get_unique_work(msg->status.enemy_id, &work, &mini);
    switch (found) {
    default:
        switch (msg->status.kind) {
        case 1:
        case 5:
            quest_enemy_spawn_req(msg->status.value_0x06, msg->status.enemy_id, msg->status.value_0x07, 0,
                        msg->status.value_0x05);
            break;
        }
        break;
    case 1:
        switch (msg->status.kind) {
        case 1:
        case 5:
            if (work->field_0x016 <= msg->status.phase) {
                if (work->field_0x004 == 0) {
                    work->field_0x004 += 1;
                    em_act_advance(work, work->field_0x00E);
                }
                if (em_die_ck(work) == 0) {
                    work->field_0x816 = work->field_0x1C0;
                    if (msg->status.kind == 5) {
                        switch (get_enemy_data(work)->kind_0x0F) {
                        case 17:
                            em_event_settle_kind17(work);
                            break;
                        case 20:
                            em_event_settle_kind20(work);
                            break;
                        case 21:
                            em_event_settle_kind21(work);
                            break;
                        case 27:
                            em_event_settle(work, 1);
                            break;
                        default:
                            em_event_settle(work, 0);
                            break;
                        }
                    } else {
                        em_event_settle(work, 0);
                    }
                }
            }
            break;
        case 2:
            if (work->field_0x016 < msg->status.phase) {
                if (work->field_0x00C == 3) {
                    if (work->field_0x004 <= 1) {
                        em_act_end(work, 0);
                    }
                } else {
                    work->field_0x016 += 1;
                    work->field_0x012 = 0;
                    em_act_advance(work, 1);
                }
            }
            break;
        case 3:
            if (work->field_0x016 == msg->status.phase) {
                u8 step = msg->status.step;

                if (work->field_0x012 <= step) {
                    work->field_0x012 = step;
                    if (work->field_0x011 == 0) {
                        work->field_0x011 = 1;
                    }
                }
            }
            break;
        }
        break;
    case 2:
        switch (msg->status.kind) {
        case 1:
        case 5:
            if (mini->phase_0x1C <= msg->status.phase && mini->state_0x08 != 3) {
                emc_mini_event_a(mini, 0, 1);
            }
            break;
        case 2:
            if (mini->phase_0x1C < msg->status.phase && mini->state_0x08 == 3) {
                emc_mini_event_b(mini, 1);
            }
            break;
        case 3:
            if (mini->phase_0x1C == msg->status.phase) {
                u8 step = msg->status.step;

                if (mini->step_0x12 <= step) {
                    mini->step_0x12 = step;
                    emc_mini_step(mini);
                }
            }
            break;
        case 4:
            if (mini->phase_0x1C == msg->status.phase && mini->step_0x12 < msg->status.step &&
                mini->state_0x08 == 4) {
                emc_mini_event_b(mini, 1);
            }
            break;
        }
        break;
    }
}

/* Builds and sends the 0x20-byte enemy-control marker message for marker `index`. */
void emc_net_send_marker(EmcWork* emc, u8 from, s32 to, u8 kind, u16 index) {
    NetEmcMarkerMsg msg;
    Marker2Rec* rec;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    rec = &emc->marker2_0x1F8[index];
    msg.field_0x00 = rec->field_0x00;
    msg.enemy_id = rec->enemy_id_0x02;
    msg.field_0x04 = rec->field_0x04;
    msg.field_0x05 = rec->field_0x05;
    msg.field_0x06 = rec->field_0x06;
    msg.field_0x07 = rec->field_0x07;
    msg.field_0x08 = rec->field_0x08;
    msg.field_0x0A = rec->field_0x0A;
    copyVec3(&msg.pos, &rec->pos_0x0C);
    msg.field_0x18 = (u16)rec->field_0x18;
    msg.field_0x1C = (u16)rec->field_0x1C;
    msg.field_0x20 = (u16)rec->field_0x20;
    broadcastSessionCommand(&msg, 0x20);
}

/* Constructs the 0x20-byte marker message's position sub-object and returns the message. */
NetEmcMarkerMsg::NetEmcMarkerMsg() {
    VEC3_ctor(&pos);
}

/* Replays a received marker on the mini enemy it belongs to, when that enemy is in its first mode. */
void emc_net_recv_marker(NetEmcMarkerMsg* msg) {
    _ENEMY_MINI_WORK* mini;
    s32 values[3];

    if ((u8)em_get_unique_work(msg->enemy_id, NULL, &mini) == 2 && mini->state_0x08 == 1) {
        values[0] = msg->field_0x18;
        values[1] = msg->field_0x1C;
        values[2] = msg->field_0x20;
        emc_marker_replay(mini, msg->field_0x05, msg->field_0x06, msg->field_0x00, msg->field_0x07, msg->field_0x0A,
                    msg->field_0x08, &msg->pos, values, 1);
    }
}

/* The client's own enemy-control sender: when the session is up and this client may send, dispatches on
 * the kind to the status or the marker builder. */
void emc_net_send(EmcWork* emc, u8 kind, u16 param) {
    u8 own;

    if (isServerSelectState() == 0) {
        return;
    }
    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (u8)my_player_no();
    switch (kind) {
    case 1:
        emc_net_send_status(emc, own, 9, kind);
        break;
    case 2:
        emc_net_send_marker(emc, own, 9, kind, param);
        break;
    }
}

/* The client's own enemy-control receiver: dispatches a received message on its kind to the status or the
 * marker applier, ignoring messages this client sent itself. */
void emc_net_recv(u8 slot, NetMsgHeader* msg) {
    s8 own;

    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (s8)my_player_no();
    if (msg->from_slot == own) {
        return;
    }
    switch (msg->kind) {
    case 1:
        emc_net_recv_status((NetEmcStatusMsg*)msg);
        break;
    case 2:
        emc_net_recv_marker((NetEmcMarkerMsg*)msg);
        break;
    }
}

/* Fills the identity record an effect-slot message carries: the slot's two keys and live byte, and the step. */
void eft_net_ident_set(EftSlot* slot, u8 step, NetEftIdent* ident) {
    ident->key_0x00 = slot->key_0x00;
    ident->key_0x01 = slot->key_0x01;
    ident->live_0x14 = slot->field_0x14;
    ident->step = step;
}

/* Returns the id of the work record the slot tracks (mode 2 keeps its index in +0x10), 0xFFFF when the index
 * is unset or the record is not in use; any other mode answers the raw +0x10 byte. */
u16 eft_slot_work_id_get(EftSlot* slot) {
    if (slot->field_0x0F == 2) {
        if (slot->field_0x10 != 0xFF) {
            _ENEMY_WORK* work = &((_ENEMY_WORK*)get_move_work_adrs(3))[slot->field_0x10];

            if (work->active != 0) {
                return work->field_0x01A;
            }
        }
        return 0xFFFF;
    }
    return slot->field_0x10;
}

/* Resolves a slot's +0x10 byte from a received mode and work id: mode 2 looks the id up among the enemies
 * (0xFF when it is unset or unknown), any other mode takes the id's low byte. */
u8 eft_slot_work_index_resolve(u8 mode, u16 work_id) {
    _ENEMY_WORK* work;

    if (mode == 2) {
        if (work_id != 0xFFFF) {
            if ((u8)em_get_unique_work(work_id, &work, NULL) != 1) {
                return 0xFF;
            }
            return work->group;
        }
        return 0xFF;
    }
    return (u8)work_id;
}

/* Builds and sends the 0x12-byte effect-slot state message: the slot's state, mode, tracked work and
 * whether its work index is the sender's own. */
void eft_net_send_state(EftSlot* slot, u8 from, s32 to, u8 kind) {
    NetEftStateMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, 0, &msg.ident);
    slot->field_0x06 = 0;
    msg.state_0x08 = slot->field_0x08;
    msg.field_0x0B = slot->field_0x0B;
    msg.mode_0x0f = slot->field_0x0F;
    msg.work_id = eft_slot_work_id_get(slot);
    msg.live_0x14 = slot->field_0x14;
    msg.field_0x36 = slot->field_0x36;
    if ((s8)slot->field_0x0C == from) {
        msg.bound = 1;
    } else {
        msg.bound = 0;
    }
    msg.field_0x3A = slot->field_0x3A;
    msg.field_0x05 = slot->field_0x05;
    broadcastSessionCommand(&msg, 0x12);
}

/* Applies a received effect-slot state message: the slot's two marks, its state (through the state setter)
 * and its mode with the work index the received id resolves to. */
void eft_net_recv_state(EftSlot* slot, NetEftStateMsg* msg) {
    slot->field_0x06 = 0;
    slot->field_0x3A = msg->field_0x3A;
    slot->field_0x36 = msg->field_0x36;
    eft_slot_state_set(slot, msg->state_0x08, NULL, msg->field_0x05);
    slot->field_0x0F = msg->mode_0x0f;
    slot->field_0x10 = eft_slot_work_index_resolve(msg->mode_0x0f, msg->work_id);
}

/* Builds and sends the 0x0E-byte effect-slot step message; the slot's step counter advances with it. */
void eft_net_send_step(EftSlot* slot, u8 from, s32 to, u8 kind) {
    NetEftStepMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    slot->field_0x06 = 0;
    msg.state_0x08 = slot->field_0x08;
    msg.field_0x0B = slot->field_0x0B;
    msg.mode_0x0f = slot->field_0x0F;
    msg.work_id = eft_slot_work_id_get(slot);
    msg.live_0x14 = slot->field_0x14;
    slot->field_0x05 = 0;
    broadcastSessionCommand(&msg, 0xE);
}

/* Applies a received effect-slot step message when it names this slot and its state: the previous mode pair
 * is kept, the received one is installed and the counters restart. */
void eft_net_recv_step(EftSlot* slot, NetEftStepMsg* msg) {
    slot->field_0x06 = 0;
    if (slot->field_0x14 == msg->live_0x14 && slot->field_0x08 == msg->state_0x08) {
        slot->field_0x11 = slot->field_0x0F;
        slot->field_0x12 = slot->field_0x10;
        slot->field_0x0F = msg->mode_0x0f;
        slot->field_0x10 = eft_slot_work_index_resolve(msg->mode_0x0f, msg->work_id);
        slot->field_0x1C = 0;
        slot->field_0x05 = 0;
    }
}

/* Builds and sends the 0x0A-byte effect-slot live message carrying the slot's live byte. */
void eft_net_send_live(EftSlot* slot, u8 from, s32 to, u8 kind) {
    NetEftLiveMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    slot->field_0x06 = 0;
    msg.kind_0x14 = slot->field_0x14;
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received effect-slot live message: the slot is re-pointed to the received key. */
void eft_net_recv_live(EftSlot* slot, NetEftLiveMsg* msg) {
    slot->field_0x06 = 0;
    eft_slot_kind_set(slot, msg->kind_0x14, 1);
}

/* Builds and sends the 0x14-byte effect-slot position message carrying the slot's position. */
void eft_net_send_pos(EftSlot* slot, u8 from, s32 to, u8 kind) {
    NetEftPosMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    slot->field_0x06 = 0;
    copyVec3(&msg.pos, &slot->pos_0x24);
    slot->field_0x1E = 0;
    broadcastSessionCommand(&msg, 0x14);
}

/* Constructs the 0x14-byte effect-slot position message's position sub-object and returns the message. */
NetEftPosMsg::NetEftPosMsg() {
    VEC3_ctor(&pos);
}

/* Applies a received effect-slot position message: the slot's position is replaced. */
void eft_net_recv_pos(EftSlot* slot, NetEftPosMsg* msg) {
    slot->field_0x06 = 0;
    copyVec3(&slot->pos_0x24, &msg->pos);
    slot->field_0x1E = 0;
}

/* Builds and sends the 0x0A-byte effect-slot work message: the slot's work index, marked confirmed when the
 * caller asked for it or the ready count is one. */
void eft_net_send_work(EftSlot* slot, u8 from, s32 to, u8 kind, u8 confirm) {
    NetEftWorkMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    slot->field_0x06 = 0;
    msg.work = slot->work_0x03;
    if (confirm == 1 || isReadyCountOne() == 1) {
        msg.confirmed = 1;
    } else {
        msg.confirmed = 0;
    }
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received effect-slot work message: when it is confirmed (or the ready count is one) the slot
 * adopts the work index and marks itself armed when that index is the receiver's own, then re-sends its
 * state if the ready count is one. */
void eft_net_recv_work(EftSlot* slot, NetEftWorkMsg* msg, u8 own) {
    u8 work;

    slot->field_0x06 = 0;
    if (msg->confirmed != 0 || isReadyCountOne() == 1) {
        work = msg->work;
        slot->work_0x03 = (s8)work;
        if ((s8)(u8)(s8)work == own) {
            slot->armed_0x02 = 1;
        } else {
            slot->armed_0x02 = 0;
        }
        if (isReadyCountOne() == 1) {
            eft_net_send(slot, 5, 1);
        }
    }
}

/* Builds and sends the 0x0A-byte effect-slot mark message carrying the slot's two marks. */
void eft_net_send_mark(EftSlot* slot, u8 from, s32 to, u8 kind) {
    NetEftMarkMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, 0, &msg.ident);
    msg.field_0x3A = slot->field_0x3A;
    msg.field_0x3B = slot->field_0x3B;
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received effect-slot mark message by forwarding its two marks. */
void eft_net_recv_mark(EftSlot* slot, NetEftMarkMsg* msg) {
    eft_slot_marks_set(slot, msg->field_0x3A, msg->field_0x3B);
}

/* Builds and sends the 0x09-byte effect-slot bind message carrying the caller's player slot. */
void eft_net_send_bind(EftSlot* slot, u8 from, s32 to, u8 kind, u8 param) {
    NetEftBindMsg msg;

    memset(&msg, 0, sizeof(msg));
    slot->field_0x06 = 0;
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    msg.work = param;
    broadcastSessionCommand(&msg, 9);
}

/* Applies a received effect-slot bind message: the slot adopts the named player slot, and re-sends its
 * state when that slot is the local client's. */
void eft_net_recv_bind(EftSlot* slot, NetEftBindMsg* msg) {
    slot->field_0x06 = 0;
    if (msg->work == (s8)my_player_no()) {
        slot->armed_0x02 = 0;
        slot->work_0x03 = (s8)msg->work;
        eft_net_send(slot, 5, 0);
    } else {
        slot->armed_0x02 = 0;
        slot->work_0x03 = (s8)msg->work;
    }
}

/* Builds and sends the 0x0A-byte effect-slot release message: the caller's flag and the slot's work index. */
void eft_net_send_release(EftSlot* slot, u8 from, s32 to, u8 kind, u8 param) {
    NetEftReleaseMsg msg;

    memset(&msg, 0, sizeof(msg));
    slot->field_0x06 = 0;
    msg.hdr.fill(from, to, kind);
    eft_net_ident_set(slot, slot->field_0x04++, &msg.ident);
    msg.flag = param;
    msg.work = slot->work_0x03;
    broadcastSessionCommand(&msg, 0xA);
}

/* Applies a received effect-slot release message: flag 0 re-sends the state when the ready count is one,
 * otherwise the named player slot is adopted (re-sending when it is the local client's and the slot is
 * not armed yet). */
void eft_net_recv_release(EftSlot* slot, NetEftReleaseMsg* msg) {
    slot->field_0x06 = 0;
    if (msg->flag == 0) {
        if (isReadyCountOne() == 1) {
            eft_net_send(slot, 8, 1);
        }
    } else if (msg->work == (s8)my_player_no()) {
        if (eft_slot_armed_ck(slot) == 0) {
            slot->work_0x03 = (s8)msg->work;
            eft_net_send(slot, 5, 0);
        }
    } else {
        slot->armed_0x02 = 0;
        slot->work_0x03 = (s8)msg->work;
    }
}

/* The client's own effect-slot sender: when the session is up, this client may send and the slot survives
 * its work, dispatches on the mode to the matching message builder. */
void eft_net_send(EftSlot* slot, u32 mode, u32 value) {
    u8 own;

    if (isServerSelectState() == 0) {
        return;
    }
    if (Pl_net_can_send() == 0) {
        return;
    }
    own = (u8)my_player_no();
    if (eft_slot_persist_ck(slot) == 0) {
        return;
    }
    switch ((u8)mode) {
    case 1:
        eft_net_send_state(slot, own, 10, mode);
        break;
    case 2:
        eft_net_send_step(slot, own, 10, mode);
        break;
    case 3:
        eft_net_send_live(slot, own, 10, mode);
        break;
    case 4:
        eft_net_send_pos(slot, own, 10, mode);
        break;
    case 5:
        eft_net_send_work(slot, own, 10, mode, value);
        break;
    case 6:
        eft_net_send_mark(slot, own, 10, mode);
        break;
    case 7:
        eft_net_send_bind(slot, own, 10, mode, value);
        break;
    case 8:
        eft_net_send_release(slot, own, 10, mode, value);
        break;
    }
}

