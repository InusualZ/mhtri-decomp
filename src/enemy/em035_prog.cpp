/* enemy/em035_prog.cpp - the em035 enemy program's tail, `.text` 0x8035F2B4..0x8035FC18 (20
 * functions / 0x964 B), extab 0x800175DC..0x80017634 and extabindex 0x80036F30..0x80036FB4.
 * Naming note: the names this file *references* in other units are still the map's generated
 * `fn_XXXXXXXX` stems (the `fn_8012*`/`fn_8014*` callee band and the lobby `fn_8021*`/`fn_8035FC*`
 * note below) - those are other lanes' to rename.  Every symbol this file DEFINES is named from its
 * own body; the derivation and its evidence are under "NAMING" below.
 *
 * WHAT IT IS.  The em035 program's sub-state handlers, continued from the registered
 * `enemy/fn_8035E034.cpp`: the `.data` table `em035_prog_tbl` (0x805ED838, 0x6C B) lists this range's
 * entry points (`em035_frame_tick` at its +0x8, then `em035_action_dispatch`, `em035_action11_effect`, `em035_kcolor_set`,
 * `em035_timer_done_ck` and `em035_part_node_init` at its last word 0x805ED8A0), and every body drives the shared
 * `_ENEMY_WORK` record (`+0x005` sub-state, `+0x020` timer, `+0x188` position, `+0x1C4` angle,
 * `+0x1E5` action, `+0x1E6` first sub-state, `+0xB14` sound handle) through the same motion arming
 * pair (`fn_8012F5B8`/`fn_8012F62C`) the neighbour above uses.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string covers the range: the
 * `.data`/`.sdata` its relocations reach are the unowned program tables (`lbl_805ED8C0`/`lbl_805ED8F8`/
 * `lbl_805ED930`) and the `0x8079B71C..0x8079B738` float pool, never a source-file-name literal.
 * 2. `dumpmap.py lookup` gives only `zz_XXXXXXXX_`.  3. The module is `enemy` from the program table
 * that lists the range's own entry points and from the `_ENEMY_WORK` record every body drives - the
 * same evidence as the registered neighbour `enemy/fn_8035E034.cpp`.
 *
 * NAMING (all 20 names are GUESSES; the map had only `fn_XXXXXXXX`).  The file is `em035_prog` for
 * the range's own evidence: the `.data` program table `em035_prog_tbl` (0x805ED838) lists these very
 * entry points, and the module's scheme is `em*` (`em_action.cpp`'s `em_act_*` band is the sibling
 * precedent).  Each symbol names what its body does, on that scheme:
 *   * `em035_frame_tick` - the per-frame step (clamps the `+0x1CC` approach weight, ticks the
 *     `+0x328`/`+0x32A` counters, tail-calls `fn_80131E00`);
 *   * `em035_arm_mot1s4_wait90` / `em035_arm_mot1s4_angle_wait90` / `em035_arm_mot1s0_wait90` /
 *     `em035_arm_mot1s0_angle_wait90` - the four arming steps, named for the motion the pair arms
 *     (`fn_8012F62C(self, 1, 4, 0)` / `(self, 1, 0, 0)`), whether they reset `+0x1C4` to 0x8000 first,
 *     and whether the wait is the 90-frame timer or `fn_8012F93C`-done (`em035_arm_mot2_exit`,
 *     `em035_arm_mot2_angle_exit` - `fn_8012F5B8(self, 2, 0, 0)`);
 *   * `em035_handlers_mot1s4` / `em035_handlers_alt` / `em035_handlers_angle` /
 *     `em035_handlers_blend` - the four second-level sub-state dispatchers, each named for the handler
 *     set it selects;
 *   * `em035_motion_done_step` - picks `fn_80127FE4`/`fn_80127F48` from the `+0x1E2` mode byte;
 *   * `em035_substate_se_start` - starts the action's sound program (`fn_801251D8` with the
 *     `0x805ED8C0`/`0x805ED8F8`/`0x805ED930` tables) from the `+0x1E6` sub-state;
 *   * `em035_blend_seq` / `em035_blend_entry` - the three-stage motion-blend sequences
 *     (`fn_80146008`/`fn_80146058`/`fn_8014610C`);
 *   * `em035_action_dispatch` - the `+0x1E5` action-id dispatch the shared interpreter calls;
 *   * `em035_action11_effect` - action 11's one-shot `eft019_set`/`se_req_pos_ps` trigger;
 *   * `em035_kcolor_set` - the model's `MHchar` K-colour override, once per record;
 *   * `em035_timer_done_ck` - whether the `+0x328`/`+0x32A` countdown the mode names has run out;
 *   * `em035_part_node_init` - seeds one part node (uniform scale, then the two flag-gated overrides).
 * A later pass with more evidence can refine any of them.
 *
 * SEAM - RE-CUT, AND WHY (the brief's range was two TUs).  `attribute.py` gave
 * 0x8035F2B4..0x80365C84 (79 functions / 27088 B) as one "capped" run, and the evidence splits it at
 * 0x8035FC18:
 *   * `em035_prog_tbl`'s last non-zero entry is `em035_part_node_init` (0x805ED8A0), and that body ends exactly
 *     at 0x8035FC18;
 *   * from 0x8035FC18 the callee mix changes completely: `fn_8035FC80`/`fn_8035FD08` drive the lobby
 *     work block `lobby_w` (`.bss` 0x806AAB44: `memset` of the 0x2000-byte buffer at `lobby_w+0xAC`,
 *     `fn_8021CBB0`, `fn_80217934`, `fn_80377664` on `lbl_80794880`), `fn_8035FC30`/`fn_803602A4` run
 *     the crafting-screen path (`seisan_data`, `fn_80217F4C`, `fn_802190FC`,
 *     `Gunner_opt_ok_ck(_EQUIP*)`, `Get_pl_type` on the two `_EQUIP` records at player+0x1D0/+0x1E8),
 *     and `fn_803602A4` reads back the pointer `fn_8035FC80` stores at buffer+0x204 - a screen record,
 *     a player record and `lobby_w`, none of them `_ENEMY_WORK`;
 *   * the run's 61 extab records split 11 + 50 at exactly the same address, and this unit's own
 *     object emits the first 11 byte-for-byte (`datagap.py --mode both --all-sections` reports no
 *     allocated-section gap), so the first half is a complete TU and the second is a fragment of a
 *     lobby-screen TU that must not be claimed with it.
 * The second half (0x8035FC18..0x80365C84, 59 functions / 24.4 KB, extab 0x80017634..0x800177C4,
 * extabindex 0x80036FB4..0x8003720C) is left unregistered and re-proposed under `lobby` (its screen
 * record and `lobby_w` view are its own); this file claims only the em035 half.
 *
 * LANGUAGE AND SECTIONS.  C++: the range reaches genuinely mangled callees through their real
 * signatures (rule 9).  Every plain `fn_` definition is `extern "C"` so it keeps the map's name
 * (playbook 42).  The lib is `enemy` (`cflags_main`) plus a file-wide `#pragma peephole off`: the
 * target keeps the unfused forms the pass folds (every `--work->timer_0x020 <= 0` is `addi` + `cmpwi`,
 * never `addic.`), and the same pass folds the `u8` increment's mask the other way - which is why the
 * sub-state bumps are compound assignments (`+= 1`) rather than `+ 1`.
 *
 * STATUS / RESIDUALS (measured with the official report metric).  20 of 20 functions written; the
 * unit scores 99.825294 fuzzy / 88.352745 code, and `datagap.py --mode both --all-sections` reports
 * no allocated-section gap (our `.text`, `extab`, `extabindex` and `.sdata2` all pair).  Two
 * residuals, both four bytes:
 *   * `em035_frame_tick` 98.54 - the two float webs are coloured the other way round: retail keeps the
 *     field value in f2 and the pool constant in f1, this build the reverse, so the `fcmpo`/`fadds`
 *     operand registers differ while every instruction and the size are already right.  Eleven source
 *     shapes were measured (`weight`/`limit` in both declaration orders, the constant as a local, the
 *     comparison and compound-assignment spellings, the nested-expression form); the best two are
 *     this one (98.54) and the reversed-declaration form (95.00, which swaps the two loads instead).
 *     `tools/m2c` (run through `tools/units/m2cinput.py`) drafts the same shape with the same f2/f0
 *     split - the allocator's web order is the residual, not the source.
 *   * `em035_part_node_init` 98.48 - retail passes the `setVec3` result straight on (`mr r4,r3`),
 *     which needs the helper's *pointer* return type.  The owner's header carries it now
 *     (`VEC3* setVec3(VEC3*, f32, f32, f32)`, docs/plan.md 6.5 rule 11); the remaining four-byte
 *     residual is the allocator's, not the spelling's.
 */

#include "types.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "unsplit/enemy.h"
#include "ef/eft019.h"
#include "gx.h"
#include "mh3_pad.h"
#include "sound/fn_800D7F54.h"
#include "sound/mhchar.h"
#include "unsplit/unknown.h"

/* The target keeps the unfused forms the peephole pass folds: every sub-state handler compares its
 * decremented `+0x020` timer as `addi` + `cmpwi` (the pass fuses them into `addic.`), and folds the
 * narrow `u8` increment's mask the other way (which is why the store is a compound assignment). */
#pragma peephole off

/* Pool literals (declared, never defined - playbook 29). */
extern "C" f32 lbl_8079B71C;
extern "C" f32 lbl_8079B720;
extern "C" f32 lbl_8079B724;
extern "C" f32 lbl_8079B728;
extern "C" f32 lbl_8079B72C;
extern "C" f32 lbl_8079B730;
extern "C" f32 lbl_8079B734;
extern "C" f32 lbl_8079B738;

/* The `.data` tables the action start hands to `fn_801251D8` (declared, never defined - the
 * unowned 0x805ED8C0 band; see `include/unsplit/enemy.h`). */
extern "C" u8 lbl_805ED8C0[];
extern "C" u8 lbl_805ED8F8[];
extern "C" u8 lbl_805ED930[];

/* The record `em035_part_node_init` seeds.  The body reaches the node at +0x34 and the three fields inside it;
 * the 0x34-byte head belongs to the caller and is left unnamed.
 * size: 0x44 (the extent this unit's body reaches) */
struct EmNode {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;  /* the three copies `copyVec3` writes */
    /* +0x0C */ u16 field_0x0C;             /* 0 at entry, then the source word's low half */
    /* +0x0E */ u8 flags_0x0E;              /* bit 0 is set once the node has been seeded */
};

struct EmPartNode {
    /* +0x00 */ u8 unused_0x00[0x34];
    /* +0x34 */ EmNode node_0x34;
};  /* size: 0x44 (the extent this unit's body reaches) */

/* The scale source `em035_part_node_init`'s third argument points at; only its +0x04 word is read.
 * size: 0x08 (the extent this unit's body reaches) */
struct EmPartSrc {
    /* +0x00 */ u8 unused_0x00[0x04];
    /* +0x04 */ u32 field_0x04;
};

/* This unit's own forward declarations (definitions follow in address order). */
extern "C" void em035_frame_tick(_ENEMY_WORK* work);
extern "C" void em035_arm_mot1s4_wait90(_ENEMY_WORK* work);
extern "C" void em035_arm_mot1s4_angle_wait90(_ENEMY_WORK* work);
extern "C" void em035_handlers_mot1s4(_ENEMY_WORK* work);
extern "C" void em035_arm_mot2_exit(_ENEMY_WORK* work);
extern "C" void em035_arm_mot1s0_wait90(_ENEMY_WORK* work);
extern "C" void em035_handlers_alt(_ENEMY_WORK* work);
extern "C" void em035_arm_mot2_angle_exit(_ENEMY_WORK* work);
extern "C" void em035_arm_mot1s0_angle_wait90(_ENEMY_WORK* work);
extern "C" void em035_handlers_angle(_ENEMY_WORK* work);
extern "C" void em035_motion_done_step(_ENEMY_WORK* work);
extern "C" void em035_substate_se_start(_ENEMY_WORK* work);
extern "C" void em035_blend_seq(_ENEMY_WORK* work);
extern "C" void em035_blend_entry(_ENEMY_WORK* work);
extern "C" void em035_handlers_blend(_ENEMY_WORK* work);
extern "C" void em035_action_dispatch(_ENEMY_WORK* work);
extern "C" void em035_action11_effect(_ENEMY_WORK* work);
extern "C" void em035_kcolor_set(_ENEMY_WORK* work);
extern "C" u32 em035_timer_done_ck(_ENEMY_WORK* work, u8 mode);
extern "C" void em035_part_node_init(struct EmPartNode* part, const nw4r::math::VEC3* vec, struct EmPartSrc* src,
                            u8 flags);

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F2B4 - steps the em035 program's approach weight and ticks the two short counters.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_frame_tick(_ENEMY_WORK* work)
{
    f32 weight = work->field_0x1CC;
    f32 limit = lbl_8079B71C;

    if (weight < limit) {
        weight = weight + lbl_8079B720;
        work->field_0x1CC = weight;
        if (weight > limit)
            work->field_0x1CC = limit;
    }
    if (work->field_0x00A != 0) {
        if (work->field_0x328 > 0)
            work->field_0x328 -= 1;
    }
    if (work->timer_0x32A > 0)
        work->timer_0x32A -= 1;
    fn_80131E00(work);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F314 - arms sub-state 1's motion pair, then runs the 90-frame wait out.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot1s4_wait90(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130478(work, 0);
        fn_8012F62C(work, 1, 4, 0);
        work->timer_0x020 = 90;
        break;
    case 1:
        if (--work->timer_0x020 <= 0)
            fn_80127F48(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F39C - the same pair with the third angle reset; the step refreshes every frame.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot1s4_angle_wait90(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130248(work);
        fn_801305C4(work);
        fn_8012F62C(work, 1, 4, 0);
        work->timer_0x020 = 90;
        fn_80136DF4(work);
        break;
    case 1:
        fn_80136DF4(work);
        if (--work->timer_0x020 <= 0)
            fn_80127FE4(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F444 - a three-way sub-state dispatch into the two arming steps above.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_handlers_mot1s4(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        em035_arm_mot1s4_wait90(work);
        break;
    case 1:
        em035_arm_mot1s4_wait90(work);
        break;
    case 2:
        em035_arm_mot1s4_wait90(work);
        break;
    case 3:
        em035_arm_mot1s4_angle_wait90(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F480 - arms sub-state 1's motion, then leaves once the motion reports done.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot2_exit(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130478(work, 0);
        fn_8012F5B8(work, 2, 0, 0);
        break;
    case 1:
        if (fn_8012F93C(work) == 1)
            fn_80127F48(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F4FC - the 90-frame variant of 0x8035F314's arming pair.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot1s0_wait90(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130478(work, 0);
        fn_8012F62C(work, 1, 0, 0);
        work->timer_0x020 = 90;
        break;
    case 1:
        if (--work->timer_0x020 <= 0)
            fn_80127F48(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F584 - a two-way sub-state dispatch into the two arming steps above.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_handlers_alt(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        em035_arm_mot2_exit(work);
        break;
    case 1:
        em035_arm_mot1s0_wait90(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F5A8 - the angle-reset arming step whose wait leaves once the motion reports done.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot2_angle_exit(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130248(work);
        fn_801305C4(work);
        fn_8012F5B8(work, 2, 0, 0);
        fn_80136DF4(work);
        break;
    case 1:
        fn_80136DF4(work);
        if (fn_8012F93C(work) == 1)
            fn_80127FE4(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F644 - the angle-reset arming step that waits 90 frames.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_arm_mot1s0_angle_wait90(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130248(work);
        fn_801305C4(work);
        fn_8012F62C(work, 1, 0, 0);
        work->timer_0x020 = 90;
        fn_80136DF4(work);
        break;
    case 1:
        fn_80136DF4(work);
        if (--work->timer_0x020 <= 0)
            fn_80127FE4(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F6EC - a two-way sub-state dispatch into the two angle-reset steps above.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_handlers_angle(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        em035_arm_mot2_angle_exit(work);
        break;
    case 1:
        em035_arm_mot1s0_angle_wait90(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F710 - picks the sub-state-1 motion step from the record's mode byte.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_motion_done_step(_ENEMY_WORK* work)
{
    if (work->field_0x1E2 == 1)
        fn_80127FE4(work);
    else
        fn_80127F48(work);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F724 - starts the action's sound/effect program from the current sub-state.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_substate_se_start(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        fn_801251D8(work, lbl_805ED8C0, 0, 0);
        break;
    case 5:
        fn_801251D8(work, lbl_805ED8C0, 1, 5);
        break;
    case 56:
        fn_801251D8(work, lbl_805ED8F8, 0, 56);
        break;
    case 57:
        fn_801251D8(work, lbl_805ED930, 1, 57);
        break;
    default:
        fn_801251D8(work, lbl_805ED8C0, 0, 0);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F7B0 - the three-stage motion arming sequence with its per-stage blend literals.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_blend_seq(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130478(work, 0);
        fn_8012F5B8(work, 1, 0, 0);
        fn_8014616C(work, 0);
        break;
    case 1:
        if (fn_80146008(1166) == 1) {
            work->state += 1;
            fn_8014619C(work);
            fn_8012F5B8(work, 2, 0, 0);
            fn_80146058(work, lbl_8079B724, lbl_8079B728, lbl_8079B72C);
            fn_8014610C(work, lbl_8079B728, lbl_8079B730, lbl_8079B728);
        }
        break;
    case 2:
        if (fn_80146008(1324) == 1) {
            work->state += 1;
            fn_8012F5B8(work, 1, 0, 0);
            fn_80146058(work, lbl_8079B734, lbl_8079B728, lbl_8079B738);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F8D4 - arms the stage-1 motion and its two blend sets, then leaves on motion done.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_blend_entry(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        fn_80130478(work, 0);
        fn_8012F5B8(work, 1, 0, 0);
        fn_80146058(work, lbl_8079B734, lbl_8079B728, lbl_8079B738);
        fn_8014610C(work, lbl_8079B728, lbl_8079B730, lbl_8079B728);
        break;
    case 1:
        if (fn_8012F93C(work) == 1)
            fn_80127F48(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F978 - a two-way sub-state dispatch into the two blend sequences above.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_handlers_blend(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        em035_blend_seq(work);
        break;
    case 1:
        em035_blend_entry(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F99C - the action-id dispatch the shared interpreter calls each frame.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_action_dispatch(_ENEMY_WORK* work)
{
    switch (work->action) {
    case 0:
        em035_handlers_mot1s4(work);
        break;
    case 1:
        em035_handlers_alt(work);
        break;
    case 3:
        em035_handlers_angle(work);
        break;
    case 10:
        em035_motion_done_step(work);
        break;
    case 11:
        em035_substate_se_start(work);
        break;
    case 13:
        em035_handlers_blend(work);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F9F0 - the action-11 sub-states' one-shot effect/SE trigger.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_action11_effect(_ENEMY_WORK* work)
{
    if (work->action != 11)
        return;
    if (work->state_sub != 0 && work->state_sub != 5 && (u8)(work->state_sub + 200) > 1)
        return;
    if (work->effect_latch_0x32C != 0)
        return;

    eft019_set(&work->pos, work->area_no, 67);
    if (work->area_no == get_now_areano())
        se_req_pos_ps(work->se_0xB14, 1, 2, &work->pos);
    work->effect_latch_0x32C = 1;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035FA94 - the model's K-colour override, applied once per record.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_kcolor_set(_ENEMY_WORK* work)
{
    if (work->field_0x00A != 0)
        return;
    if (work->kcolor_latch_0x32D != 0)
        return;

    _GXColor color;
    ((MHchar*)work->char_0x024)->getTevKColor(0, GX_KCOLOR3, &color);
    color.r = 190;
    color.g = 190;
    color.b = 147;
    ((MHchar*)work->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
    work->kcolor_latch_0x32D = 1;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035FB18 - reports whether the countdown the mode names has run out.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u32 em035_timer_done_ck(_ENEMY_WORK* work, u8 mode)
{
    switch (mode) {
    case 0:
        if (work->field_0x328 <= 0)
            return 1;
        break;
    case 1:
        if (work->timer_0x32A <= 0)
            return 1;
        break;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035FB60 - seeds one part node: its uniform scale, then the two optional overrides.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void em035_part_node_init(EmPartNode* part, const nw4r::math::VEC3* vec, EmPartSrc* src, u8 flags)
{
    EmNode* node = &part->node_0x34;
    nw4r::math::VEC3 scale;

    setVec3(&scale, lbl_8079B728, lbl_8079B728, lbl_8079B728);
    copyVec3(&node->vec_0x00, &scale);
    node->field_0x0C = 0;
    if (flags & 2)
        copyVec3(&node->vec_0x00, vec);
    if (flags & 4)
        node->field_0x0C = (u16)src->field_0x04;
    node->flags_0x0E |= 1;
}
