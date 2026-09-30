/*
 * enemy/em_act_step.cpp - the enemy work record's action-step band: the step function of each of the
 * record's first four action ids, the motion each of their phases arms, and the record's own class (its
 * constructor, vtable and destructor halves) with the effect-offset and seat-aim helpers.  `.text`
 * 0x8032C920..0x8033041C (74 functions, 15100 B).  Registered once, at its final home, from
 * `proposal/8032C920_fn_8032C920.cpp` (docs/plan.md 12) - that proposal's range 0x8032C920..0x80334568
 * was one `--max-bytes` cut over TWO translation units, and the seam re-draw of 2026-09-26 split it
 * here; `enemy/em_pl_frame.cpp` is the other half (0x8033041C..0x80334568, 48 functions / 16716 B).
 *
 * MODULE AND NAME (brief section 2, evidence order).  No `__FILE__` string is reachable from the unit
 * (its whole `.data` run 0x805DFC9C..0x805E0510 is jump tables and the class vtable `lbl_805E04E0` - the
 * only printable bytes in it are two 4-byte coincidences) and `tools/symbols/dumpmap.py lookup` answers
 * `zz_XXXXXXXX_` for every symbol, so class 1 and 2 decide nothing.  Class 3 decides it: the record every
 * function takes in r3 is the enemy work record - the range calls its API with that pointer as the
 * subject (`em_frame_check__FP11_ENEMY_WORKUsff`, `em_act_ck__FP11_ENEMY_WORKUcUc`,
 * `em_die_ck__FP11_ENEMY_WORK`, `em_after_frame_check__FP11_ENEMY_WORKUsff`,
 * `em_get_mot_no__FP11_ENEMY_WORK`, 67 call sites), and every field it reads on that pointer is one
 * `include/enemy/ENEMY_WORK.h` names for `_ENEMY_WORK`: +0x005 the per-motion state, +0x188 `pos`,
 * +0x1BC/+0x1C0/+0x1C4 the rotation triple, +0x1CC the seat-preference float, +0x328 the per-slot
 * block, +0x338, +0x834 the "already seated" byte, +0xB14 the `_se_w` handle.  The bracketing
 * registered units are `hud/fn_80324F7C.c` (below) and `enemy/em_pl_frame.cpp` (the other half of this
 * proposal's range, above); this unit's own class (the vtable `lbl_805E04E0` its constructor
 * `em_work_ctor` installs) derives from the class whose
 * constructor is `em_res_user_data_ctor` and whose vtable `lbl_805A1368` lives in the enemy band - eleven slots,
 * this range overrides three (the destructor `fn_803300CC`, `fn_8032FFEC`, `fn_8032FFF4`).  Class 4 (a
 * `fn_XXXXXXXX` file stem) was the registration's name for want of anything better; the naming pass
 * below replaced it with what the band *is*.
 *
 * NAMING (merger lane, 2026-09-26: the batch's own symbols, the gate's rule 7 refusal).  Every address
 * of the range is a bare `fn_XXXXXXXX` row in the map and `tools/symbols/dumpmap.py lookup` answers
 * `zz_XXXXXXXX_` for all of them, so no name here comes from evidence class 1 or 2 - each one is
 * **derived from the function's own body** (the fields it reads on the record, the API calls it makes,
 * what it returns) and fits the module's own `em_<noun>_<verb>` scheme (`em_act_ck`, `em_frame_check`,
 * `em_get_mot_no`; the neighbouring band `enemy/em_action.cpp` names its own functions `em_act_hold`,
 * `em_act_mot21`, `em_act_frame_ck`).  All 34 definitions were renamed in
 * `config/RMHE08/symbols.txt` and here in one edit (`symedit.py rename-batch`; playbook 31/48 - objdiff
 * pairs by name, so half a rename measures 0 %).  The scheme, and the datum behind each name:
 *
 *  - `em_act_step_<n>` = the record's step function for action id `n`: the band's own dispatcher
 *    `fn_8032FA88` switches on `+0x1E5 action` (the field `include/enemy/ENEMY_WORK.h` names as the id
 *    `em_act_ck` matches) and its jump table's cases 0..3 call these four, so each one is the step of
 *    that action state and each switches on `+0x1E6 state_sub` for its phase;
 *  - `em_act_arm_<motion>[...]` = one phase of such a step, which arms a motion through the record's
 *    own setters (`em_mot_set(self, motion, mode, 0)`, `em_mot_set_ck`, `em_mot_set_blend`) and then ends the
 *    step when the motion's frame check (`em_mot_end_ck`, `em_turn_to_target`, `em_approach_step`, `em_frame_check`)
 *    reports done; `_hit<a>_<b>` is the pair handed to the damage handler `em_state_set`, `_f<frames>`
 *    the frame count the arm sets, `_mot_mode` an arm whose motion is the caller's mode argument;
 *  - the rest are named for the record's own state machine and helpers, not for a motion.
 *
 * Each is a **guess** the bodies support and a later pass may refine; the per-function comment above
 * every definition states the same evidence in full.  Old map stem -> name:
 *
 *   fn_8032C920 -> em_eff_offset_set         builds the (0, 0, 10.0f) offset, rotates it by the
 *                                            record's own `rot_y` and hands the vector to `fn_80130350`
 *   fn_8032C994 -> em_eff_offset_clr         zeroes `handle_0x328.vec` and its `field_0x0C` word
 *   fn_8032C9D8 -> em_act_face_away          gates on the player being in play and on <3 set act bits,
 *                                            then points `rot_y` at the reverse bearing (returns 1)
 *   fn_8032CC2C -> em_eff_ground_set         looks the +0x1A id up with `fn_801421E4` and places the
 *                                            effect at the ground record's position and rotation
 *   fn_8032CEA8 -> em_work_ctor              the record's class constructor: base ctor `em_res_user_data_ctor`,
 *                                            then stores this unit's vtable `lbl_805E04E0`
 *   fn_8032CEE4 -> em_work_dtor_del          the vtable's deleting-destructor half (empty in retail)
 *   fn_8032CEE8 -> em_work_noop              the vtable's other empty virtual stub
 *   fn_8032CEEC -> em_act_die_step           while `em_die_ck` refuses it, steps `+0x1CC` up to its
 *                                            ceiling once `+0x338` is 1
 *   fn_8032CF68 -> em_act_arm_mot1           action 0 phases 0-2: `em_mot_set_ck(self, 1, 6, 0)`, ends on
 *                                            `em_mot_end_ck`
 *   fn_8032CFE4 -> em_act_arm_mot201         action 0 phase 3: the two effect resets, then motion 201
 *   fn_8032D074 -> em_act_step_0             the step function of action id 0: `state_sub` 0/1/2 ->
 *                                            `em_act_arm_mot1`, 3 -> `em_act_arm_mot201`
 *   fn_8032D0B0 -> em_act_arm_mot2           action 1 phase 0: `em_mot_set(self, 2, 6, 0)`
 *   fn_8032D12C -> em_act_arm_mot_mode       action 1 phases 1-4: arms the motion the caller's mode
 *                                            selects (0->3, 1->4, 2->6, 3->9)
 *   fn_8032D210 -> em_act_arm_mot11          action 1 phase 5: the two motion counters, then motion 11
 *   fn_8032D2A0 -> em_act_arm_mot1_hit1_1    action 1 phase 6: motion 1, then the scan of the `+0x454`
 *                                            per-attacker values hands `em_state_set(self, 1, 1)`
 *   fn_8032D3F4 -> em_act_arm_mot1_f20       action 1 phase 7: `em_mot_set_blend(self, 1, 20, 0, 1)` - motion 1
 *                                            with a 20-frame check
 *   fn_8032D480 -> em_act_step_1             the step function of action id 1: `state_sub` 0..7 (the
 *                                            eight-arm switch whose table is this unit's `.data`)
 *   fn_8032D4D8 -> em_act_arm_mot5           action 2 phase 0: motion 5 with a `bits_0x1EC`-scaled speed
 *                                            factor and a 150-frame countdown
 *   fn_8032D5C4 -> em_act_arm_mot7           action 2 phase 1: the same step for motion 7, 120 frames
 *   fn_8032D6B0 -> em_act_arm_mot12          action 2 phase 2: motion 12, waits 512 frames
 *   fn_8032D730 -> em_act_arm_mot13          action 2 phase 3: motion 13, waits 1024 frames
 *   fn_8032D7B0 -> em_act_arm_mot10          action 2 phase 4: motion 10, the 2048-frame check first
 *   fn_8032D838 -> em_act_arm_mot114         action 2 phase 5: motion 114 (0x72), 120-frame countdown
 *   fn_8032D8C0 -> em_act_step_2             the step function of action id 2: `state_sub` 0..5
 *   fn_8032D914 -> em_act_arm_mot202         action 3 phase 0: the +0xCA resets, then motion 202
 *   fn_8032D9A4 -> em_act_arm_mot201_hit7_2  action 3 phase 1: motion 201, the attacker scan every 16th
 *                                            step hands `em_state_set(self, 7, 2)`
 *   fn_8032DAE4 -> em_act_arm_mot203_204     action 3 phases 2/3: motion 203, or 204 when the mode is 1
 *   fn_8032DBAC -> em_act_arm_mot205         action 3 phase 4: motion 205, 120-frame countdown and a
 *                                            64-frame `em_approach_step` check
 *   fn_8032DC70 -> em_act_arm_mot207         action 3 phase 5: motion 207, waits 1024 frames
 *   fn_8032DF44 -> em_act_arm_mot210_hit3_10 action 3 phase 11: motion 210, then `em_state_set(self, 3,
 *                                            10)` and `em_eff_offset_set` at the end of every step
 *   fn_8032DFD4 -> em_act_arm_mot201_hit7_6  action 3 phase 18: motion 201, the same scan with
 *                                            `em_state_set(self, 7, 6)`
 *   fn_8032E108 -> em_act_arm_mot203_hit3_20 action 3 phase 19: motion 203 and `em_state_set(self, 3, 20)`
 *   fn_8032E190 -> em_act_step_3             the step function of action id 3: `state_sub` 0..5, 10, 11,
 *                                            18, 19 (the sparse table behind its `cmplwi 21`)
 *   fn_8032E1E8 -> em_seat_aim_ck            1 / -1 / 0 for "the seat has a player and
 *                                            `em_act_face_away` turned the record" / "refused" / "no seat"
 *
 * Naming note: the file names every symbol it *defines* (the table above); the escape that remains
 * covers precisely the names it *references* in other units - `em_mot_set`, `em_move_mode_set`, `em_state_set`
 * and ~50 more callee stems whose owners have not been named yet, plus the rows of this range's own
 * unwritten tail (`fn_8032DD04`, `fn_8032E23C`, `fn_803303E0`, ...), which belong to the pass that
 * writes their bodies.  Renaming the callee stems is a cross-unit batch in ten owner units, not this
 * lane's change (the same escape `enemy/em_action.cpp` records).  Every definition below is `extern
 * "C"` so objdiff pairs it by the map's name (playbook 42); a C++ definition would mangle and measure
 * 0 %.
 *
 * SEAM - SETTLED (seam re-draw, 2026-09-26): the proposal's range is two translation units and the cut
 * is 0x8033041C.  Three instruments agree, and none of them is the behavioural argument the first note
 * used:
 *  - the extabindex run names its own functions - entry 57 (at 0x800357A8) is `fn_8033041C`, so this
 *    unit takes entries 0..56 and the other half 57..98;
 *  - the `.sdata2` pool run 0x8079B108..0x8079B2AC is two objects' pools, cut at 0x8079B20C |
 *    0x8079B210: this unit's labels are referenced only by functions 0x8032C920..0x80330128, the other
 *    half's only by 0x803305F8..0x80334398, and no label is shared.  MWCC's u32->f32 magic
 *    `0x4330000080000000` is emitted twice inside the run - 0x8079B140 for `em_act_arm_mot5`/`em_act_arm_mot7`
 *    and 0x8079B228 for `fn_803305F8` - and 0.0f / 0.5f / 1.0f / 10.0f / 20.0f / 30.0f / 60.0f / 0.8f /
 *    -30.0f each occur once per half.  A pool emits one copy of a constant per object (this unit's own
 *    object uses the magic in two functions and emits one 8-byte entry) and the linker merges nothing
 *    (that magic occurs >= 40x in the DOL's `.sdata2`), so two copies mean two emitters;
 *  - the `.data` run splits at the same place: this unit's table fragment ends with the class vtable
 *    0x805E04E0 (0x30 B) and the other half's begins at 0x805E0510 with six objects `fn_803305F8`
 *    alone references.
 * `fn_803303E0` (60 B) ends exactly on the cut.  The behavioural argument the first note recorded is
 * still true but it is a hint, not the seam: this unit's own `em_act_arm_mot201_hit7_2`/`em_act_arm_mot201_hit7_6` already scan
 * the player work records, so "the first half drives the enemy work" was already wrong at 0x8032D9A4.
 *
 * SECTIONS.  `.text` 0x8032C920..0x8033041C.  The unit owns the extab run 0x80016464..0x8001662C
 * (57 x 8 B) and the extabindex run 0x800354FC..0x800357A8 (57 x 12 B) - 17 of its 74 functions carry
 * no extab record (`em_work_dtor_del`, `em_act_step_0`, `em_act_step_1`, `em_act_step_2`, `em_act_step_3`,
 * `fn_8032ED78`, `fn_8032F2FC`, `fn_8032FA34`, `fn_8032FCD8`, `fn_8032FEF0`, `fn_8032FFEC`,
 * `fn_803303E0`, ...) - plus the `.ctors` word 0x8056F3A0 -> `fn_80330128`, its static initializer
 * (still unwritten).  `.data` 0x805DFC9C..0x805E0510 **is claimed**: it is this unit's own table run
 * (the jump tables, the per-function tables of `fn_8032EDE0`, `fn_8032F014`, `fn_8032F550`,
 * `fn_8032FA88`, and the class vtable `lbl_805E04E0`), it starts exactly where the previous band's last
 * object (`em026_prog_tbl`, 0x805DFC30..0x805DFC9C) ends and ends exactly where the other half's first
 * table object (0x805E0510) starts, so it is a complete fragment - and the claim was measured before
 * and after (build green, every row of the written functions unchanged).  `.sdata2`
 * 0x8079B108..0x8079B210 (this unit's pool half) is deliberately NOT claimed: a `.sdata2` claim links
 * only while the object emits no pool of its own and ours emits the compiler's 8-byte magic (playbook
 * 23/58), so its labels stay declarations (playbook 29).  The other half's pool half
 * 0x8079B210..0x8079B2AC is not this unit's and is not declared here either - `include/unsplit/enemy.h`
 * now carries this half's words only.
 *
 * FLAGS.  `cflags_main` (Wii/1.3, `-O3 -inline noauto -Cpp_exceptions on`), the bracketing enemy units'
 * set - the range's callees and its extab presence agree with it, so nothing per-unit is added.
 *
 * THE RECORD.  `_EM_CHARA_WORK` below is this unit's view of the record: the offsets
 * `include/enemy/ENEMY_WORK.h` does not name yet (+0x354 as an `f32`, +0x565/+0x566/+0x56B, +0x5C6,
 * +0x454) are kept here because that header is shared and a worker may not edit it; the outbox carries
 * the `shared-file` request that folds them in.  The view is a union of the two shared views of the same
 * record (`_ENEMY_WORK` in `include/enemy/ENEMY_WORK.h`, `_PLW` in `include/pl.h`), which is why callees
 * that declare `_PLW*` are called through a cast of the same pointer.
 *
 * RESIDUALS.  34 of the unit's 74 functions are written (10732 B of 15100), 24 of them byte-identical;
 * the unit reads 34.69 % fuzzy / 16.93 % matched code, and `build/RMHE08/main.dol` stays OK.  The redraw
 * changed two source spellings and nothing else: `em_act_arm_mot201_hit7_2` and `em_act_arm_mot201_hit7_6` compared against
 * `lbl_8079B2A0`, which is the OTHER half's copy of 0.8 - a reference across the seam, impossible once
 * the range is two TUs - so both now use this unit's own `lbl_8079B178` (0.8).  Both measured identical
 * before and after (98.25 % / 98.18 %), so the score does not say which copy the body wants; the
 * target's `em_act_arm_mot201_hit7_2` loads `lbl_8079B150` (800.0) there and calls `em_state_set` at +0x1170, so the
 * body's constant is a decompilation defect for the next pass - recorded, not guessed at.  The ten
 * written functions that are not byte-identical:
 *
 *  - em_eff_offset_set 92.93 %: the 12-byte copy into the outgoing local is the only difference - retail
 *    pairs two `lwz` before two `stw` (two registers), ours alternates one `lwz`/`stw` per word.  A
 *    `VEC3` copy, a `_CP_VECTOR` copy and the cast form all measure the same.
 *  - em_act_face_away 97.61 %: register colouring only - retail holds the record/player in r29/r30 and the
 *    +0x328 block in r31, ours in r30/r31/r29; the instruction stream and the frame are equal.
 *  - em_act_arm_mot1_hit1_1 98.76 %, em_act_arm_mot201_hit7_2 98.25 %, em_act_arm_mot201_hit7_6 98.18 %: the attacker scan.  Retail
 *    advances the move-work pointer by 0xB20 at the loop tail (`addi r3,r3,0xb20`) and re-forms the
 *    per-attacker float address from the index; ours keeps the base pointer (`work->`) and indexes the
 *    +0x454 array.  Measured: `work[i]` (the stride spelling) costs 92.4 % on `em_act_arm_mot1_hit1_1`, so the
 *    `work->` spelling is kept.
 *  - em_act_arm_mot5 93.22 %, em_act_arm_mot7 93.22 %, em_act_arm_mot205 95.92 %: one argument-materialisation order
 *    - retail loads f1 before `li r4, 0` for the trailing `em_approach_start(self, 0, -200.0f)`, ours after
 *    (the same three instructions, reordered).  The constants of both are the pool labels
 *    `lbl_8079B138` (the `f32` argument) and the record's own `bits_0x1EC * 0.1` factor.
 *  - em_act_arm_mot203_204 99.80 %: one register - retail masks the mode into r0 at the first test
 *    (`clrlwi r0, r4, 24`) where ours masks into the saved copy.
 *  - em_act_step_3 99.95 %: one immediate - retail's jump-table range check is `cmplwi r0, 21` and ours
 *    `cmplwi r0, 19`, i.e. the source switch's largest case was 21 where this one's is 19 (the table's
 *    gap entries are the default label in both); adding an empty `case 21: break;` measured the
 *    same, so the source likely had a case there with a body this reconstruction has not found yet.
 *
 * Unit-level gaps (measured with `tools/units/datagap.py` after the redraw):
 *  - `.sdata2` ours-extra 8 B: the `0x43300000_80000000` u32->f32 magic MWCC pools for
 *    `(f32)(bits_0x1EC & 3)`.  Retail's copy is the pool word at 0x8079B140, inside this unit's
 *    unclaimed pool half; the claim would break the link while our object emits a pool of its own
 *    (playbook 23/58), so it stays unclaimed.  This is the unit's one ours-extra row.
 *  - `.data` 2164 B / `.rela.data` 1056 B target-extra: the claim is active and the run is the unit's -
 *    the object emits only the tables the written functions use (112 B today).
 *  - `.ctors` 4 B / `.rela.ctors` 12 B target-extra: the static initializer `fn_80330128` is unwritten.
 *  - `.text` 15100 vs 5304 B, extab 456 vs 224 B, extabindex 684 vs 336 B target-extra: 40 functions
 *    are unwritten; the next in address order is `fn_8032E23C` (0x1F8, the largest of the remaining
 *    rows), then `fn_8032E434` (0x118) and `fn_8032E54C` (0x2A4).
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_act_step.h"
#include "unsplit/enemy.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_8028F66C.h"
#include "Pl/pl_master.h"

/* Retail keeps the unfused form of this band's narrowing/bit folds (`and rN, rN, rM` + a separate
 * `cmpwi` where `-O3`'s peephole makes one `and.` record form) - the same finding the sibling bands
 * `enemy/fn_802F5138.cpp`, `ai/fn_802C5D10.cpp` and `light/light.cpp` record (playbook 39). */
#pragma peephole off

/* Retail keeps `fmuls` + `fadds` where our default `-fp_contract on` fuses them into one `fmadds`
 * (playbook 40: measured on `em_act_arm_mot5`/`em_act_arm_mot7`, whose speed factor is the fused site). */
#pragma fp_contract off

#ifdef __cplusplus
extern "C" {
#endif

/* The out-of-range callees whose owner's header does not declare them yet - each spelling is the
 * callee's own body (the same gap `camera/fn_802B5C58.cpp` and `ai/fn_802C5D10.cpp` recorded). */
void fn_80051B7C(void* out, const void* in, f32 scale, s32 a);
void em_action_finish_fall(struct _ENEMY_WORK* self);
void fn_8032DD04(struct _EM_CHARA_WORK* self);
f32 calcVecDistXZ(const void* a, const void* b);
void fn_8012E694(struct _ENEMY_WORK* self);
void fn_80136DF4(struct _ENEMY_WORK* self);
void fn_800FC0D4(_CP_VECTOR* dst, const _CP_VECTOR* src);
u32 quest_sub_state_end_ck(u32 a);
u32 fn_803B88A8(void);
void fn_80130350(struct _ENEMY_WORK* self, void* vec);
void fn_8027D3F0(struct _PLW* self, u8 a);
void fn_8012E664(struct _ENEMY_WORK* self);
void* em_res_user_data_ctor(void* self);
u32 fn_801421E4(u16 id, EmGroundRec* rec);
void fn_80125F54(EmGroundRec* rec);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* The enemy work API this band calls, at C++ scope so the front-end reproduces the map's mangled name
 * (rule 9); each spelling is the callee's own body. */
u32 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
s32 em_act_ck(struct _ENEMY_WORK* self, u8 a, u8 b);
s32 em_die_ck(struct _ENEMY_WORK* self);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);

/* The 16-byte per-slot handle block at +0x328: the effect offset `rotVecY` rotates in place and the
 * state's own stage word beside it.  Both are reached through one pointer in the target, so they are
 * one record here too (rule 3: size from the target's `addi rN, self, 0x328` + `stw .., 12(rN)`).
 * size: 0x10 */
struct EmHandleBlock {
    /* +0x00 */ nw4r::math::VEC3 vec;
    /* +0x0C */ u32 field_0x0C;
};

/* The record every function in this range takes in r3 (docs/plan.md 6.5 rules 3-5: size from the
 * range's largest access - +0xB14, the `_se_w` handle - and from `include/enemy/ENEMY_WORK.h`'s 0xB18;
 * every field ascending and named from its use).  Offset +0x000 is the class vtable the constructor
 * `em_work_ctor` stores, so the class's object *is* this record.
 * size: 0xB18 */
struct _EM_CHARA_WORK {
    /* +0x000 */ union {                /* the object's first four bytes carry both readings: the class
                                         * vtable `em_work_ctor` stores here, and the header's own
                                         * `active`/`group`/`team` bytes are the same four bytes. */
        void** vtable;
        struct {                       /* size: 0x04 */
            /* +0x000 */ u8 field_0x000;
            /* +0x001 */ u8 field_0x001;
            /* +0x002 */ u8 group;     /* the enemy group/entry index `fn_8027D3F0` is handed */
            /* +0x003 */ u8 team;
        };
    };
    /* +0x004 */ u8 field_0x004;        /* below 2 the record still counts as alive */
    /* +0x005 */ u8 state;              /* the per-motion step the action machines switch on */
    /* +0x006 */ u8 unused_0x006[0x009 - 0x006];
    /* +0x009 */ u8 field_0x009;        /* 2/3 select the joint the effect is placed on */
    /* +0x00A */ u8 unused_0x00A[0x01A - 0x00A];
    /* +0x01A */ u16 field_0x01A;       /* the ground-record id `fn_801421E4` looks up */
    /* +0x01C */ u8 unused_0x01C[0x020 - 0x01C];
    /* +0x020 */ s32 field_0x20;       /* the step's own countdown; its low 5 bits gate the scan */
    /* +0x024 */ u8 char_0x024[0x40];   /* the embedded `MHchar` base the `em_` frame checks hand on */
    /* +0x064 */ u8 unused_0x064[0x188 - 0x064];
    /* +0x188 */ nw4r::math::VEC3 pos;  /* one position, the effect anchor */
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
    /* +0x1BC */ union {
        _CP_VECTOR rot;                /* the rotation triple `fn_800FC0D4` copies in whole */
        struct {                       /* size: 0x0C */
            /* +0x1BC */ u32 rot_x;
            /* +0x1C0 */ u32 rot_y;     /* the angle `rotVecY` turns the effect by */
            /* +0x1C4 */ u32 rot_z;
        };
    };
    /* +0x1C8 */ u32 unused_0x1C8;
    /* +0x1CC */ f32 field_0x1CC;       /* the per-step speed the state steps clamp */
    /* +0x1D0 */ u8 unused_0x1D0[0x1E6 - 0x1D0];
    /* +0x1E6 */ u8 state_sub;          /* the sub-state the dispatchers switch on */
    /* +0x1E7 */ u8 unused_0x1E7[0x1EC - 0x1E7];
    /* +0x1EC */ u16 bits_0x1EC;        /* the low two bits scale the action's speed factor */
    /* +0x1EE */ u8 unused_0x1EE[0x328 - 0x1EE];
    /* +0x328 */ EmHandleBlock handle_0x328;
    /* +0x338 */ u8 field_0x338;
    /* +0x339 */ u8 unused_0x339[0x354 - 0x339];
    /* +0x354 */ f32 field_0x354;       /* the model scale the act entry hands the model layer */
    /* +0x358 */ u8 unused_0x358[0x454 - 0x358];
    /* +0x454 */ f32 values_0x454[4];   /* the per-attacker values the step loop scans */
    /* +0x464 */ u8 unused_0x464[0x565 - 0x464];
    /* +0x565 */ u8 field_0x565;        /* the primary-act latch */
    /* +0x566 */ u8 field_0x566;        /* armed once the entry block has run */
    /* +0x567 */ u8 unused_0x567[0x5C6 - 0x567];
    /* +0x5C6 */ u8 field_0x5C6;
    /* +0x5C7 */ u8 unused_0x5C7[0xA16 - 0x5C7];
    /* +0xA16 */ u8 field_0xA16;        /* 0xFF means "no seat id yet" */
    /* +0xA17 */ u8 unused_0xA17[0xA34 - 0xA17];
    /* +0xA34 */ _PLW* plw_0xA34;       /* the seat's player work the aiming step reads */
    /* +0xA38 */ u8 field_0xA38;        /* nonzero: the seat is not aimable */
    /* +0xA39 */ u8 unused_0xA39[0x7FB];
    /* +0x834 */ u8 field_0x834;        /* the "already seated" byte the step tests against 1 */
    /* +0x835 */ u8 unused_0x835[0xB14 - 0x835];
    /* +0xB14 */ struct _se_w* se_0xB14;
};

#endif

/* Rotates the effect's local offset vector by the work record's own y angle, then hands it to the
 * effect placer. */
extern "C" void em_eff_offset_set(_EM_CHARA_WORK* self)
{
    nw4r::math::VEC3 offset;
    _CP_VECTOR out;

    VEC3_ctor(&offset);
    offset.x = lbl_8079B108;
    offset.y = lbl_8079B108;
    offset.z = lbl_8079B110;
    rotVecY(&offset, self->rot_y);
    out = *(_CP_VECTOR*)&offset;
    fn_80130350((_ENEMY_WORK*)self, &out);
}

/* Clears the effect offset block and the state word next to it. */
extern "C" void em_eff_offset_clr(_EM_CHARA_WORK* self)
{
    EmHandleBlock* block = &self->handle_0x328;

    setVector3(&block->vec, lbl_8079B108, lbl_8079B108, lbl_8079B108);
    block->field_0x0C = 0;
}

/* Faces the work record away from the player: bails unless the player is in play and fewer than three
 * of the player's act bits are set, then turns the record's y angle to the reverse bearing and stores
 * the sub-step. */
extern "C" u32 em_act_face_away(_EM_CHARA_WORK* self, _PLW* pl)
{
    nw4r::math::VEC3* offset = &self->handle_0x328.vec;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 seed;
    nw4r::math::VEC3 target;
    u32 ang_a;
    u32 ang_b;
    u32 bits;
    s32 i;
    s32 count;

    if (pl == NULL) {
        return 0;
    }
    if (pl->slot_active == 0) {
        return 0;
    }
    if (Pl_master_ck(pl) != 1) {
        return 0;
    }
    if (quest_sub_state_end_ck(1) != 0) {
        return 0;
    }
    if (fn_803B88A8() != 0) {
        return 0;
    }

    bits = pl->field_0x3B0;
    count = 0;
    for (i = 0; i < 32; i++) {
        if ((bits & (1u << i)) != 0) {
            count++;
        }
    }
    if (count >= 3) {
        return 0;
    }

    VEC3_ctor(&dir);
    setVec3(&seed, lbl_8079B108, lbl_8079B108, lbl_8079B110);
    copyVec3(offset, &seed);

    subVec3(&target, &self->pos, &pl->vec_0x03C);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    rotVecY(offset, (u16)(ang_a - pl->field_0x058));

    subVec3(&target, &pl->vec_0x03C, &self->pos);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    self->rot_y = ang_a - pl->field_0x058;
    self->handle_0x328.field_0x0C = 3;
    fn_8027D3F0(pl, self->group);
    return 1;
}

/* Places the effect at the work record's position when the ground record for its +0x1A id is found,
 * and copies that record's rotation into the record. */
extern "C" void em_eff_ground_set(_EM_CHARA_WORK* self)
{
    EmGroundRec rec;

    fn_80125F54(&rec);
    if (fn_801421E4(self->field_0x01A, &rec) != 0) {
        fn_80051B7C(&self->pos, &rec.pos_0x08, lbl_8079B114, 0);
        fn_800FC0D4(&self->rot, (_CP_VECTOR*)&rec.field_0x14);
    }
}

/* The class constructor: runs the base constructor, then installs this unit's vtable. */
extern "C" _EM_CHARA_WORK* em_work_ctor(_EM_CHARA_WORK* self)
{
    em_res_user_data_ctor((_ENEMY_WORK*)self);
    self->vtable = (void**)lbl_805E04E0;
    return self;
}

/* The class destructor's deleting half, called from the vtable's slot 2 by `fn_803300CC`. */
extern "C" void em_work_dtor_del(void)
{
}

/* The range's other empty stub - a virtual override whose body the target does not emit either. */
extern "C" void em_work_noop(void)
{
}

/* Steps the death action: while `em_die_ck` refuses it and the +0x834 byte is set, advances the
 * +0x1CC speed toward its ceiling. */
extern "C" void em_act_die_step(_EM_CHARA_WORK* self)
{
    if (em_die_ck((_ENEMY_WORK*)self) != 0) {
        return;
    }
    if (self->field_0x834 == 1) {
        fn_8012E664((_ENEMY_WORK*)self);
    }
    if (self->field_0x338 == 1) {
        f32 speed = self->field_0x1CC;

        if (speed < lbl_8079B11C) {
            f32 next = speed + lbl_8079B120;

            self->field_0x1CC = next;
            if (next > lbl_8079B11C) {
                self->field_0x1CC = lbl_8079B11C;
            }
        }
    }
}

/* Arms the +0x06 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot1(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set_ck((_ENEMY_WORK*)self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xC9 action (with the two effect resets that precede it), then ends the step. */
extern "C" void em_act_arm_mot201(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set_ck((_ENEMY_WORK*)self, 201, 6, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the two step functions this band holds. */
extern "C" void em_act_step_0(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot1(self);
        break;
    case 1:
        em_act_arm_mot1(self);
        break;
    case 2:
        em_act_arm_mot1(self);
        break;
    case 3:
        em_act_arm_mot201(self);
        break;
    }
}

/* Arms the +0x02 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot2(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the action matching the caller's mode on the first step, then ends the step when the action's
 * frame check passes; mode 1 first re-arms the record's own sub-state. */
extern "C" void em_act_arm_mot_mode(_EM_CHARA_WORK* self, u32 action)
{
    if ((action & 0xFF) == 1) {
        fn_80131E00((_ENEMY_WORK*)self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        switch ((u8)action) {
        case 0:
            action = 3;
            break;
        case 1:
            action = 4;
            break;
        case 2:
            action = 6;
            break;
        case 3:
            action = 9;
            break;
        }
        em_mot_set((_ENEMY_WORK*)self, action, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Advances the record's two motion counters, then arms the +0x0B action or ends the step. */
extern "C" void em_act_arm_mot11(_EM_CHARA_WORK* self)
{
    fn_80131DB4((_ENEMY_WORK*)self);
    fn_80131DF4((_ENEMY_WORK*)self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 11, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            fn_8012E694((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Advances the record's motion counters and scans the per-attacker values for one still inside the
 * band, handing the first such attacker to the damage handler. */
extern "C" void em_act_arm_mot1_hit1_1(_EM_CHARA_WORK* self)
{
    fn_80131DB4((_ENEMY_WORK*)self);
    fn_80131DF4((_ENEMY_WORK*)self);
    fn_80136D14((_ENEMY_WORK*)self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 1, 0, 0);
        fn_801303EC((_ENEMY_WORK*)self, lbl_8079B124);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        fn_801303EC((_ENEMY_WORK*)self, lbl_8079B124);
        if ((self->field_0x20 & 0x1F) != 0) {
            break;
        }
        max = get_move_work_max(2);
        work = (_PLW*)get_move_work_adrs(2);
        for (i = 0; i < (u16)max; i++) {
            f32 value;

            if (work == NULL) {
                continue;
            }
            if (work->slot_active == 0) {
                continue;
            }
            if (work->field_0x3B0 == 0) {
                continue;
            }
            value = self->values_0x454[i];
            if (value > lbl_8079B128) {
                continue;
            }
            if (value < lbl_8079B108) {
                continue;
            }
            em_state_set((_ENEMY_WORK*)self, 1, 1);
            return;
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0x14 action with a 20-frame frame check, then ends the step when that check passes. */
extern "C" void em_act_arm_mot1_f20(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set_blend((_ENEMY_WORK*)self, 1, 20, 0, 1);
        break;
    case 1:
        if (em_frame_check((_ENEMY_WORK*)self, 0, lbl_8079B12C, lbl_8079B108) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state: each arm is one of this band's step functions, and the table
 * MWCC emits for the eight arms is this unit's own `.data` (0x805DFC9C, 8 entries). */
extern "C" void em_act_step_1(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot2(self);
        break;
    case 1:
        em_act_arm_mot_mode(self, 0);
        break;
    case 2:
        em_act_arm_mot_mode(self, 1);
        break;
    case 3:
        em_act_arm_mot_mode(self, 2);
        break;
    case 4:
        em_act_arm_mot_mode(self, 3);
        break;
    case 5:
        em_act_arm_mot11(self);
        break;
    case 6:
        em_act_arm_mot1_hit1_1(self);
        break;
    case 7:
        em_act_arm_mot1_f20(self);
        break;
    }
}

/* Arms the +0x05 action with a speed factor from the record's own two low bits and a 150-frame
 * countdown, then ends the step when the frame check passes or the countdown runs out. */
extern "C" void em_act_arm_mot5(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 5, 4, 0);
        em_mot_speed_set((_ENEMY_WORK*)self,
                    lbl_8079B130 + lbl_8079B134 * (f32)(self->bits_0x1EC & 3));
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B138, 0);
        self->field_0x20 = 150;
        break;
    case 1:
        if (em_approach_step((_ENEMY_WORK*)self, 0, 128) == 1 || --self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same step as `em_act_arm_mot5` for the +0x07 action, with its own speed factor and 120 frames. */
extern "C" void em_act_arm_mot7(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 7, 4, 0);
        em_mot_speed_set((_ENEMY_WORK*)self,
                    lbl_8079B148 + lbl_8079B14C * (f32)(self->bits_0x1EC & 3));
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B138, 0);
        self->field_0x20 = 120;
        break;
    case 1:
        if (em_approach_step((_ENEMY_WORK*)self, 0, 128) == 1 || --self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0C action and waits for its 512-frame check. */
extern "C" void em_act_arm_mot12(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 12, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((_ENEMY_WORK*)self, 512) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0D action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot13(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 13, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((_ENEMY_WORK*)self, 1024) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0A action: its 2048-frame check runs first, then the action's own frame check. */
extern "C" void em_act_arm_mot10(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 10, 4, 0);
        break;
    case 1:
        em_turn_to_target((_ENEMY_WORK*)self, 2048);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x72 action with a 120-frame countdown, then ends the step when the countdown runs out. */
extern "C" void em_act_arm_mot114(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 114, 4, 0);
        self->field_0x20 = 120;
        break;
    case 1:
        if (--self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the six step functions above. */
extern "C" void em_act_step_2(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot5(self);
        break;
    case 1:
        em_act_arm_mot7(self);
        break;
    case 2:
        em_act_arm_mot12(self);
        break;
    case 3:
        em_act_arm_mot13(self);
        break;
    case 4:
        em_act_arm_mot10(self);
        break;
    case 5:
        em_act_arm_mot114(self);
        break;
    }
}

/* The +0xCA action's step: the two resets and the motion setter first, then the same frame check. */
extern "C" void em_act_arm_mot202(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 202, 6, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_2(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 201, 0, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        fn_80136DF4((_ENEMY_WORK*)self);
        if ((self->field_0x20 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > lbl_8079B178) {
                    continue;
                }
                em_state_set((_ENEMY_WORK*)self, 7, 2);
                return;
            }
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0xCB/+0xCC action - the mode picks between them - then ends the step at the frame check. */
extern "C" void em_act_arm_mot203_204(_EM_CHARA_WORK* self, u32 action)
{
    if ((u8)action == 1) {
        fn_80131E00((_ENEMY_WORK*)self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, (u16)(203 + ((action & 0xFF) == 1)), 0, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xCD action with a 120-frame countdown, then ends the step at the frame check. */
extern "C" void em_act_arm_mot205(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 205, 4, 0);
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B124, 0);
        self->field_0x20 = 120;
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_approach_step((_ENEMY_WORK*)self, 0, 64) == 1 || --self->field_0x20 <= 0) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xCF action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot207(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 207, 4, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_turn_to_target((_ENEMY_WORK*)self, 1024) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xD2 action, and on the frame check hands the target to the damage handler; the effect
 * placement runs at the end of every step. */
extern "C" void em_act_arm_mot210_hit3_10(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 210, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_state_set((_ENEMY_WORK*)self, 3, 10);
        }
        break;
    }
    em_eff_offset_set(self);
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_6(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 201, 4, 0);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        if ((self->field_0x20 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > lbl_8079B178) {
                    continue;
                }
                em_state_set((_ENEMY_WORK*)self, 7, 6);
                return;
            }
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0xCB action, then hands the target to the damage handler at the frame check. */
extern "C" void em_act_arm_mot203_hit3_20(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 203, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_state_set((_ENEMY_WORK*)self, 3, 20);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the band's step functions; unlisted states do nothing. */
extern "C" void em_act_step_3(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot202(self);
        break;
    case 1:
        em_act_arm_mot201_hit7_2(self);
        break;
    case 2:
        em_act_arm_mot203_204(self, 0);
        break;
    case 3:
        em_act_arm_mot203_204(self, 1);
        break;
    case 4:
        em_act_arm_mot205(self);
        break;
    case 5:
        em_act_arm_mot207(self);
        break;
    case 10:
        fn_8032DD04(self);
        break;
    case 11:
        em_act_arm_mot210_hit3_10(self);
        break;
    case 18:
        em_act_arm_mot201_hit7_6(self);
        break;
    case 19:
        em_act_arm_mot203_hit3_20(self);
        break;
    }
}

/* 1 while the record's seat is armed and its player is close enough for the aim step, -1 while the
 * seat is armed but the aim is refused, 0 when there is no seat to aim. */
extern "C" s32 em_seat_aim_ck(_EM_CHARA_WORK* self)
{
    if (self->field_0xA16 != 0xFF && self->field_0xA38 == 0) {
        if (em_act_face_away(self, self->plw_0xA34) == 1) {
            return 1;
        }
        return -1;
    }
    return 0;
}
