/* enemy/em020_prog.cpp - the em020 enemy program, `.text` 0x8036CF64..0x80375084 (57 functions /
 * 0x8120 B), extab 0x800178E4..0x80017A5C (47 records) and extabindex 0x800373BC..0x800375F0
 * (47 x 12 B).  Each of the three runs is exactly the gap the bracketing split objects leave:
 * `auto_fn_8036CE78_text`'s record ends at 0x800178E4/0x800373BC and `auto_fn_80375084_text`'s
 * begins at 0x80017A5C/0x800375F0.
 *
 * WHAT IT IS.  The `.data` program table `em020_prog_tbl` (0x805EE098, 0x70 B, `scope:global`)
 * lists seven of this range's entry points - 0x8036E2BC, 0x8036E320, 0x8036E6B8, 0x80372D58,
 * 0x8036E570, 0x8036E574 and 0x803733BC - exactly the way `em035_prog_tbl` lists the registered
 * `enemy/em035_prog.cpp`'s, and every body drives the shared `_ENEMY_WORK` record through
 * `em_frame_check`/`em_after_frame_check`/`em_get_mot_no`/`em_act_ck`/`em_area_ck`, the motion
 * arming pair `em_move_mode_set`/`fn_8012F504`/`fn_8012F5C4` and the nw4r math helpers
 * (`setVector3`, `mulVecMat`, `rotVecY`, `calcVecAng2`/`calcVecAngX`), with the joint/effect
 * queries `get_joint_wmat_em`/`get_em_scale`/`get_em_chg_scale`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string: a relocation sweep of
 * all 47 split objects finds no `.c`/`.cpp` literal - the range's whole data reference set is the
 * `.sdata2` pool run 0x8079B820..0x8079BC64, the `.data` run 0x805EE108..0x805EE428 and the
 * `.rodata` labels 0x80570A20/60/AA0.  2. `dumpmap.py lookup` answers only `zz_XXXXXXXX_`
 * placeholders for every function in the range.  3. The module is `enemy` and the file is
 * `em020_prog`: the map's own global `em020_prog_tbl` (a sibling of `em001/em003/em008/em033/
 * em035/em019_prog_tbl`) lists this range's handlers by address, and the range's callee profile is
 * the enemy band's (`em_*`, `fn_8012*`/`fn_8013*`); the file name follows the registered
 * `enemy/em035_prog.cpp`'s scheme for the same kind of table.
 *
 * SEAM: UNPROVEN, and it is the one thing this registration cannot settle.  Neither edge has any
 * decisive class-1 evidence (no `__FILE__` string exists anywhere in the band), and the
 * `.sdata2` ordered partition only supports the cut without proving it:
 *   * 0x8079B81C (the neighbour below, proposal/80366618) | 0x8079B820..0x8079BC60 (this range,
 *     86 single-referrer labels, no inversion) | 0x8079BC64 (the neighbour above, Q118).  The
 *     entries either side are disjoint and ordered, which is the reliable class - but a single
 *     object's pool is *also* ordered by first use, so an ordered partition is consistent with
 *     both one TU and two adjacent ones (playbook 54's "candidate, never proof").
 *   * `em020_prog_tbl`'s non-null entries straddle the right edge (0x80375084, 0x80375290,
 *     0x803753A0, 0x80375424, 0x80375494 all lie in the next two proposals).  That is NOT a
 *     must-link: `em035_prog_tbl` (0x805ED838) also lists entry points from two separate registered
 *     TUs (`enemy/fn_8035E034.cpp` and `enemy/em035_prog.cpp`), so an enemy program spanning
 *     several TUs is the module's normal shape.
 *   * The range holds two visibly different code groups.  0x8036CF64..0x8036E26C is the head: ten
 *     bodies that draw through the 2D library (`get_lsp_data`/`get_menu_lsp_tbl`/`get_str_tbl`/
 *     `draw_sprite_*`/`draw_font*`), read the quest text ids (`lb_quest_name_get`/`lb_quest_msg_get`)
 *     and own no `.sdata2` pool entry and no `.data` item at all; five of them are called from
 *     `fn_8036CC44` in the proposal below and three from the 0x802A2DB8/0x802A37F4 menu band, and
 *     their `this` record is NOT `_ENEMY_WORK` (`fn_8036DCD0`/`fn_8036DD34` touch +0x04/+0x08/+0x14
 *     as bytes and a halfword pair, where `_ENEMY_WORK` has single bytes and an `f32` at +0x1AC/+0x1B0).
 *     0x8036E2BC.. is the program proper: it owns all 86 pool labels and the whole `.data` run, and
 *     `em020_prog_tbl`'s first entry is exactly 0x8036E2BC.  A re-cut at 0x8036E2BC is requested in
 *     this batch's `config_requests`; until it is measured the whole brief range is registered here
 *     as one unit, and only the program half is written (see RESIDUALS).
 *
 * DATA (measured, not claimed).  This unit's own `.data` is 0x805EE108..0x805EE428 (0x320 B): sixteen
 * items that tile exactly and are referenced only from this range (`lbl_805EE108` 0xD8 by 0x80373010,
 * `jumptable_805EE1E0` 0x70 by 0x8036F8E4, `jumptable_805EE250` 0x20 by 0x80370368,
 * `jumptable_805EE270` 0x70 by 0x803714B0, `lbl_805EE2E0`..`lbl_805EE3A8` by 0x803714B0,
 * `lbl_805EE3A8`/`_805EE3D0` by 0x80372DAC and `lbl_805EE3F8` by 0x8036E534).  Neither section is
 * claimed, for two measured reasons:
 *   * `.data` - the run is an island in the unclaimed 0x805EDAE0.. chunk whose neighbours are other
 *     units' items (`em020_prog_tbl` at 0x805EE098 before it, `lbl_805EE428` from 0x803759C4 and
 *     `jumptable_805EE490`/`_805EE4B8` after it), and playbook 53's refinement forbids claiming
 *     several runs of one section while the bytes between them belong to another unit.
 *   * `.sdata2` 0x8079B820..0x8079BC64 - 86 entries, every one solely referenced here, so it is the
 *     private case playbook 58 allows; but the claim needs the object to emit the run and this unit
 *     is far from byte-identical, so it would promise a section our object does not produce
 *     (playbook 23's `ELF_gen.c` 2802 failure).  The bodies name the map's own pool symbols
 *     (`extern`, playbook 29 - never defined), so the rows pair by name meanwhile.
 *
 * LANGUAGE AND FLAGS.  C++: the range reaches genuinely mangled callees
 * (`em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`, ...) through their
 * real signatures (rule 9), and every plain `fn_XXXXXXXX` definition is `extern "C"` so it keeps the
 * map's name (playbook 42).  Lib `enemy` (`cflags_main`); the target objects carry 47 extab records
 * and 47 extabindex entries, and `cflags_main` is already the lib's exceptions setting.
 *
 * Naming note: references only to other units' unrenamed `fn_XXXXXXXX` symbols (checked with a
 * relocation sweep over `build/RMHE08/obj/`: every `fn_` this file calls - the 0x8012xxxx/0x8013xxxx
 * enemy band, the 0x802Axxxx menu band and the 0x8036xxxx siblings - is defined outside this range).
 * The symbols this file defines are named.
 *
 * STATUS AND RESIDUALS (official report metric; `datagap.py --unit main/enemy/em020_prog
 * --all-sections` for the data gap).  12 of the 57 functions are written and every one of them is at
 * or above the 80 % bar - nine byte-identical:
 *   em020_nop 100.0, em020_angle_step_to_zero 100.0, em020_timers_tick 100.0,
 *   em020_arm_mot1_wait20 100.0, em020_substate_dispatch 100.0, em020_arm_mot3_wait50 100.0,
 *   em020_arm_mot1_wait58 100.0, em020_arm_mot1_facing_wait77 100.0,
 *   em020_arm_mot1_effects_wait78 100.0;
 *   em020_arm_mot1_turn_wait79 96.72, em020_arm_mot1_side_wait80 92.46,
 *   em020_arm_approach_mot1_wait55 92.41.
 * Unit: 5.67 % fuzzy / 1136 of 33056 code bytes (matched_code counts the 100 % rows only).
 *   * The three near-misses are instruction-identical and differ only in *argument materialisation
 *     order* (first divergence recorded, not a percentage): turn_wait79 and approach_wait55 both want
 *     `lfs`/`lis` issued before the integer argument where this compiler issues the integer `li`/`lis`
 *     first; side_wait80 wants `cntlzw`+`extrwi`+`neg`+`addi 0x51` for `0x51 - (right == 0)` where we
 *     emit `subfic r0, r0, 0x51`.  All three were probed with the local hoisted, with the ternary
 *     polarity swapped and with the pool labels `const`/non-`const`; none of the six shapes moved the
 *     divergence, so the residual is this compiler's scheduling, not the source.
 *   * 45 functions are still unwritten, so they pair at 0 %: the two giants `fn_803733BC` (0x1CC8 B)
 *     and `fn_803716CC` (0xD24 B), the head group's ten 2D-library bodies (0x8036CF64..0x8036E26C -
 *     they need the head record's own class, which is not `_ENEMY_WORK` and which belongs to the other
 *     side of the 0x8036E2BC seam), and the remaining em020 handlers.  Their addresses and sizes are
 *     in `symbols.txt`; this header is not a body inventory.
 *   * `.data` and `.sdata2` are unclaimed for the reasons above, so every row that loads a pool
 *     constant is judged on the constant's name (`extern f32 lbl_8079Bxxxx`), never on a value this
 *     file defines.  Our object emits no `.data`/`.sdata`/`.sdata2`/`.rodata` at all (the `--all-sections`
 *     row lists only `.shstrtab` on the `ours-extra` side), so no `ours-extra` data defect exists;
 *     `datagap.py --flip-blockers` does not list this unit.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/math.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"

/* `stage/fn_802B2AA0.h` used to open a file-wide `#pragma peephole off` that this unit picked up by
 * including it.  Measured: with the leak gone this unit drops (unit fuzzy 5.672 -> 5.650;
 * `em020_arm_approach_mot1_wait55` 92.41 -> 91.01, `em020_arm_mot1_side_wait80` 92.46 -> 91.23), so
 * the unit owns the pass and states it here.  A codegen pragma in the shared header is a matching
 * hazard (stylelint rule 10). */
#pragma peephole off

/* The `.sdata2` pool half this range reads (0x8079B820..0x8079BC60, 86 single-referrer entries that
 * only this unit's functions load).  Declared, never defined - playbook 29/58: a definition would
 * make MWCC emit the named constant *and* a pool copy, so the section would grow instead of
 * pairing.  The section itself is left to its auto data unit (see the header's DATA block). */
extern "C" {
extern f32 lbl_8079B83C; /* 0.0 */
extern f32 lbl_8079B854; /* the approach scale `fn_80136D4C` takes */
extern f32 lbl_8079B858;
extern f32 lbl_8079B85C;
extern f32 lbl_8079B874;
extern f32 lbl_8079B87C;
extern f32 lbl_8079B8A4;
extern f32 lbl_8079B8C4;
extern f32 lbl_8079B8EC;
extern f32 lbl_8079B8F0;
}

#ifdef __cplusplus
extern "C" {
#endif

/* Steps the record's +0x1BC rotation angle half a word toward zero and hands the record on.  The
 * angle is a `s16` in the low halfword of a 32-bit field: values inside +/-0x40 collapse to 0 and
 * everything else moves by 0x40 in the direction of the sign. */
void em020_angle_step_to_zero(_ENEMY_WORK* self)
{
    u32 ang = self->field_0x1BC;
    u16 half = (u16)ang;

    if (half < 0x8000) {
        if (half < 0x40)
            self->field_0x1BC = 0;
        else
            self->field_0x1BC = ang - 0x40;
    } else if (half > 0xFFC0) {
        self->field_0x1BC = 0;
    } else {
        self->field_0x1BC = ang + 0x40;
    }

    fn_80133C3C(self);
}

/* Ticks the two em020 timers: the +0x334 countdown always, and the +0x33E one only while the map
 * lookup reports the record's `area_no` 2 and the interpreter's byte stack is at 6.  The +0x338
 * byte is a per-frame latch of "the map is 7 and the area is 3". */
void em020_timers_tick(_ENEMY_WORK* self)
{
    if (fn_802B0668(self->field_0x1E0) == 7 && self->area_no == 3)
        self->em020_0x328.latch_0x338 = 1;
    else
        self->em020_0x328.latch_0x338 = 0;

    if (self->em020_0x328.timer_0x334 > 0)
        self->em020_0x328.timer_0x334 -= 1;

    if (fn_802B0668(self->field_0x1E0) == 7 && self->area_no == 2 && self->stack_0x961[0] == 6) {
        if (self->em020_0x328.timer_0x33E > 0)
            self->em020_0x328.timer_0x33E -= 1;
    }
}

/* The record's two-step wake-up: it hands the record to the shared per-frame tick, steps the
 * +0x1BC angle, and then either arms the 0x14-frame pose (step 0) or waits for the motion to report
 * done and releases the record into the next action (step 1). */
void em020_arm_mot1_wait20(_ENEMY_WORK* self)
{
    fn_80133C3C(self);
    em020_angle_step_to_zero(self);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The per-frame byte the +0x1E6 sub-state makes `em020_arm_mot1_wait20` run for each of its five steps, with
 * every other sub-state falling straight through. */
void em020_substate_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot1_wait20(self);
        break;
    case 1:
        em020_arm_mot1_wait20(self);
        break;
    case 2:
        em020_arm_mot1_wait20(self);
        break;
    case 4:
        em020_arm_mot1_wait20(self);
        break;
    case 5:
        em020_arm_mot1_wait20(self);
        break;
    default:
        break;
    }
}

/* The same two-step wake-up for the second half of the pose: step 0 arms a 0x32/0x28-frame motion
 * (kind 3), step 1 waits for it and releases the record. */
void em020_arm_mot3_wait50(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, 0x32, 0x28, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The three-step wake-up of the pose whose closing approach depends on the argument: step 0 arms a
 * 0x37/0x14-frame motion and re-aims the model (`fn_80134004`), storing one of the two approach
 * floats at +0x378 for mode 1 and mode 2; step 1 waits for the 0x80-frame window and the motion
 * helper to agree, then arms the closing 0x2f/0x28 pose; step 2 waits and releases the record. */
void em020_arm_approach_mot1_wait55(_ENEMY_WORK* self, u8 mode)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, 0x37, 0x14, 0, 1);

        switch (mode) {
        default:
            fn_80134004(self, 0x12, lbl_8079B83C);
            break;
        case 1:
            fn_80134004(self, 0x12, lbl_8079B83C);
            self->value_0x378 = lbl_8079B858;
            break;
        case 2:
            fn_80134004(self, 0x12, lbl_8079B83C);
            self->value_0x378 = lbl_8079B85C;
            break;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state += 1;
            fn_8012F504(self, 0x2f, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x3a/0x0a-frame pose's two-step wake-up. */
void em020_arm_mot1_wait58(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, 0x3a, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4d/0x14-frame pose's wake-up: step 1 keeps the record facing the target
 * (`em_frame_check(self, 1, 0.0.., 0.0)` re-aims it with `fn_80133C50`) and then waits for the
 * motion to report done. */
void em020_arm_mot1_facing_wait77(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, 0x4d, 0x14, 0, 1);
        fn_80129668(self, 0, 0xa);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B874, lbl_8079B83C) == 0) {
            fn_80133C50(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4f/0x0a-frame pose's wake-up: step 1 re-aims the record with `fn_80136D4C` and closes the
 * pose with `fn_80133C50` as soon as either of the two frame windows reports done, then re-arms the
 * 0x10000-step turn. */
void em020_arm_mot1_turn_wait79(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, 0x4f, 0xa, 0, 1);
        fn_80129668(self, 0, 1);
        break;
    case 1: {
        if (em_frame_check(self, 2, lbl_8079B8EC, lbl_8079B83C) == 1 ||
            em_frame_check(self, 3, lbl_8079B8F0, lbl_8079B8C4) == 1) {
            fn_80136D4C(self, lbl_8079B854);
            fn_80133C50(self, 0x50);
        }
        f32 lo = lbl_8079B8EC;
        f32 hi = lbl_8079B8F0;
        fn_80133E3C(self, 0x10000, lo, hi);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* The 0x4e/0x0a-frame pose's wake-up: step 0 arms the motion and the two 8/0x10-part effect slots,
 * step 1 re-aims and closes the pose once the frame window reports done. */
void em020_arm_mot1_effects_wait78(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4e, 0xa, 0);
        fn_8012933C(self, 0, 0xb, 8);
        fn_8012933C(self, 1, 0xc, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B8A4, lbl_8079B83C) == 0) {
            fn_80136D4C(self, lbl_8079B854);
            fn_80133C50(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The two-directional 0x50/0x51-frame pose's wake-up: the argument picks the motion (0x51 when set,
 * 0x50 otherwise) and the sign of the 0x4000-step turn `fn_80133E3C` re-arms every frame. */
void em020_arm_mot1_side_wait80(_ENEMY_WORK* self, u8 right)
{
    Vec3 keys;

    VEC3_ctor(&keys);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F504(self, (u16)(0x51 - (right == 0)), 0x14, 0, 1);
        fn_80129668(self, 0, 0x11);
        break;
    case 1: {
        f32 lo = lbl_8079B87C;
        f32 hi = lbl_8079B8C4;
        fn_80133E3C(self, right == 0 ? 0x4000 : -0x4000, lo, hi);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* One handler slot of `em020_prog_tbl` that does nothing - the table's index 4. */
void em020_nop(_ENEMY_WORK* self)
{
    (void)self;
}

#ifdef __cplusplus
}
#endif
