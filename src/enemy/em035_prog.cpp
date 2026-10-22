/* enemy/em035_prog.cpp - the em035 enemy's program: its per-frame step, the motion-arming sub-state handlers and
 *   their dispatchers, the action dispatch and the part-node seeding.
 * RANGE. .text 0x8035F2B4-0x8035FC18 (20 functions); .data 0x805ED838-0x805ED950, .sdata2 0x8079B71C-0x8079B740,
 *   extab, extabindex.  The program's head (0x8035F060-0x8035F2B4: the table's init, empty and event slots, and the
 *   pool word 0x8079B718 its init reads) sits in `enemy/em033_prog.cpp` and is declared by `enemy/em035_init.h`.
 * SEAM. The right edge: `em035_prog_tbl`'s last entry is `em035_part_node_init`, whose body ends at 0x8035FC18; from
 *   there `lobby/lb_screen_step.cpp` drives `lobby_w` and the crafting screen, and the band's extab records split
 *   11 + 50 at the same address (this object emits the first 11).
 * NAMES. The file from `em035_prog_tbl` (0x805ED838), which lists this range's entry points; every symbol it defines
 *   is a GUESS from its body on the module's `em*` scheme: `arm_mot<M>s<S>` arms `em_mot_set_ck(self, M, S, 0)`,
 *   `_angle` first resets `+0x1C4` to 0x8000, `_wait90` waits the 90-frame timer and `_exit` `em_mot_end_ck`.
 *   GUESS (the sound programs, named for the sub-state that plays them): em035_se_rec_sub0/_sub56/_sub57,
 *   em035_se_tbl_sub0/_sub56/_sub57; the table's head slots em035_init, em035_noop, em035_event_notify.
 * RESIDUALS. none in the bodies, `.data` or `.sdata2` (the pool is 0x20 B against the claim's 0x24: the last word is
 *   the link's alignment fill before the next unit's 8-byte constant).
 * SHAPES. File-wide `#pragma peephole off` (retail compares each decremented `+0x020` timer as `addi` + `cmpwi`) and
 *   `#pragma pool_data off` (each sound table loads by name); the sub-state bumps are compound assignments (`+= 1`),
 *   which keeps the `u8` mask retail has; the pool constants are literals in first-use order; `em035_frame_tick` steps
 *   the field in place (a `weight`/`limit` local pair swaps f1/f2); `em035_part_node_init` hands `setVec3`'s result
 *   straight to `copyVec3`.
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
#include "enemy/em_se_record.h"
#include "enemy/em035_init.h" /* em035_init / em035_noop / em035_event_notify */

/* Retail compares each decremented `+0x020` timer as `addi` + `cmpwi`, which the peephole pass fuses into
 * `addic.`. */
#pragma peephole off
/* Retail loads each sound table by name (its own `lis`/`addi`), never through one `.data` base. */
#pragma pool_data off

/* The node `em035_part_node_init` seeds at +0x34 of the caller's record (`EmPartNode`).
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


/* The em035 program table: the entry points the shared enemy program driver calls.  Slot names follow
 * `Em024ProgTbl` (`enemy/em024_ai.cpp`), a GUESS from the slots the two tables share. */
struct Em035ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* work, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* work);
    /* +0x0C */ void (*action_dispatch_0x0C)(_ENEMY_WORK* work);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* work, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* work);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* work);
    /* +0x24 */ u32 (*timer_done_ck_0x24)(_ENEMY_WORK* work, u8 mode);
    /* +0x28 */ u32 pad_0x28[0x10];
    /* +0x68 */ void (*part_node_init_0x68)(struct EmPartNode* part, const nw4r::math::VEC3* vec,
                                            struct EmPartSrc* src, u8 flags);
    /* +0x6C */ u32 pad_0x6C;
}; /* size: 0x70 */

extern "C" Em035ProgTbl em035_prog_tbl = {
    0,
    em035_init,
    em035_frame_tick,
    em035_action_dispatch,
    em035_noop,
    em035_event_notify,
    em035_action11_effect,
    0,
    em035_kcolor_set,
    em035_timer_done_ck,
    {0},
    em035_part_node_init,
    0,
};

/* The sound programs `em035_substate_se_start` hands to `em_se_tbl_play_alt`: one record each, wrapped in a
 * count/wait/play/end table.  Names are a GUESS from the sub-state that plays them. */
EmSeRecord em035_se_rec_sub0 = {1, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em035_se_tbl_sub0[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(4)},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(2)},
    {0x00, {0, 0, 0}, &em035_se_rec_sub0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em035_se_rec_sub56 = {1, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em035_se_tbl_sub56[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(4)},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(2)},
    {0x00, {0, 0, 0}, &em035_se_rec_sub56},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em035_se_rec_sub57 = {1, 0, 0, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em035_se_tbl_sub57[4] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(4)},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(2)},
    {0x00, {0, 0, 0}, &em035_se_rec_sub57},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x8035F2B4 (0x60): steps the em035 program's approach weight and ticks the two short counters. */
extern "C" void em035_frame_tick(_ENEMY_WORK* work)
{
    if (work->field_0x1CC < 1.0f) {
        work->field_0x1CC += 0.01f;
        if (work->field_0x1CC > 1.0f)
            work->field_0x1CC = 1.0f;
    }
    if (work->field_0x00A != 0) {
        if (work->field_0x328 > 0)
            work->field_0x328 -= 1;
    }
    if (work->timer_0x32A > 0)
        work->timer_0x32A -= 1;
    em_fx_flag2_set(work);
}

/* 0x8035F314 (0x88): arms sub-state 1's motion pair, then runs the 90-frame wait out. */
extern "C" void em035_arm_mot1s4_wait90(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set_ck(work, 1, 4, 0);
        work->timer_0x020 = 90;
        break;
    case 1:
        if (--work->timer_0x020 <= 0)
            em_action_finish(work);
        break;
    }
}

/* 0x8035F39C (0xA8): the same pair with the third angle reset; the step refreshes every frame. */
extern "C" void em035_arm_mot1s4_angle_wait90(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set_ck(work, 1, 4, 0);
        work->timer_0x020 = 90;
        em_ground_snap(work);
        break;
    case 1:
        em_ground_snap(work);
        if (--work->timer_0x020 <= 0)
            em_action_finish_fall(work);
        break;
    }
}

/* 0x8035F444 (0x3C): a three-way sub-state dispatch into the two arming steps above. */
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

/* 0x8035F480 (0x7C): arms sub-state 1's motion, then leaves once the motion reports done. */
extern "C" void em035_arm_mot2_exit(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 2, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1)
            em_action_finish(work);
        break;
    }
}

/* 0x8035F4FC (0x88): the 90-frame variant of 0x8035F314's arming pair. */
extern "C" void em035_arm_mot1s0_wait90(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set_ck(work, 1, 0, 0);
        work->timer_0x020 = 90;
        break;
    case 1:
        if (--work->timer_0x020 <= 0)
            em_action_finish(work);
        break;
    }
}

/* 0x8035F584 (0x24): a two-way sub-state dispatch into the two arming steps above. */
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

/* 0x8035F5A8 (0x9C): the angle-reset arming step whose wait leaves once the motion reports done. */
extern "C" void em035_arm_mot2_angle_exit(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set(work, 2, 0, 0);
        em_ground_snap(work);
        break;
    case 1:
        em_ground_snap(work);
        if (em_mot_end_ck(work) == 1)
            em_action_finish_fall(work);
        break;
    }
}

/* 0x8035F644 (0xA8): the angle-reset arming step that waits 90 frames. */
extern "C" void em035_arm_mot1s0_angle_wait90(_ENEMY_WORK* work)
{
    work->field_0x1C4 = 0x8000;
    switch (work->state) {
    case 0:
        work->state += 1;
        em_fall_height_get(work);
        em_fall_start(work);
        em_mot_set_ck(work, 1, 0, 0);
        work->timer_0x020 = 90;
        em_ground_snap(work);
        break;
    case 1:
        em_ground_snap(work);
        if (--work->timer_0x020 <= 0)
            em_action_finish_fall(work);
        break;
    }
}

/* 0x8035F6EC (0x24): a two-way sub-state dispatch into the two angle-reset steps above. */
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

/* 0x8035F710 (0x14): picks the sub-state-1 motion step from the record's mode byte. */
extern "C" void em035_motion_done_step(_ENEMY_WORK* work)
{
    if (work->field_0x1E2 == 1)
        em_action_finish_fall(work);
    else
        em_action_finish(work);
}

/* 0x8035F724 (0x8C): starts the action's sound/effect program from the current sub-state. */
extern "C" void em035_substate_se_start(_ENEMY_WORK* work)
{
    switch (work->state_sub) {
    case 0:
        em_se_tbl_play_alt(work, em035_se_tbl_sub0, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt(work, em035_se_tbl_sub0, 1, 5);
        break;
    case 56:
        em_se_tbl_play_alt(work, em035_se_tbl_sub56, 0, 56);
        break;
    case 57:
        em_se_tbl_play_alt(work, em035_se_tbl_sub57, 1, 57);
        break;
    default:
        em_se_tbl_play_alt(work, em035_se_tbl_sub0, 0, 0);
        break;
    }
}

/* 0x8035F7B0 (0x124): the three-stage motion arming sequence with its per-stage blend literals. */
extern "C" void em035_blend_seq(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 1, 0, 0);
        em_demo_reset(work, 0);
        break;
    case 1:
        if (em_demo_time_ck(1166) == 1) {
            work->state += 1;
            em_demo_enable(work);
            em_mot_set(work, 2, 0, 0);
            em_demo_pos_set(work, -497.0f, 0.0f, -1698.373f);
            em_demo_rot_set(work, 0.0f, 9.75f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(1324) == 1) {
            work->state += 1;
            em_mot_set(work, 1, 0, 0);
            em_demo_pos_set(work, -530.3455f, 0.0f, -1702.6453f);
        }
        break;
    }
}

/* 0x8035F8D4 (0xA4): arms the stage-1 motion and its two blend sets, then leaves on motion done. */
extern "C" void em035_blend_entry(_ENEMY_WORK* work)
{
    switch (work->state) {
    case 0:
        work->state += 1;
        em_move_mode_set(work, 0);
        em_mot_set(work, 1, 0, 0);
        em_demo_pos_set(work, -530.3455f, 0.0f, -1702.6453f);
        em_demo_rot_set(work, 0.0f, 9.75f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1)
            em_action_finish(work);
        break;
    }
}

/* 0x8035F978 (0x24): a two-way sub-state dispatch into the two blend sequences above. */
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

/* 0x8035F99C (0x54): the action-id dispatch the shared interpreter calls each frame. */
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

/* 0x8035F9F0 (0xA4): the action-11 sub-states' one-shot effect/SE trigger. */
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

/* 0x8035FA94 (0x84): the model's K-colour override, applied once per record. */
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

/* 0x8035FB18 (0x48): reports whether the countdown the mode names has run out. */
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

/* 0x8035FB60 (0xB8): seeds one part node: its uniform scale, then the two optional overrides. */
extern "C" void em035_part_node_init(EmPartNode* part, const nw4r::math::VEC3* vec, EmPartSrc* src, u8 flags)
{
    EmNode* node = &part->node_0x34;
    nw4r::math::VEC3 scale;

    copyVec3(&node->vec_0x00, setVec3(&scale, 0.0f, 0.0f, 0.0f));
    node->field_0x0C = 0;
    if (flags & 2)
        copyVec3(&node->vec_0x00, vec);
    if (flags & 4)
        node->field_0x0C = (u16)src->field_0x04;
    node->flags_0x0E |= 1;
}
