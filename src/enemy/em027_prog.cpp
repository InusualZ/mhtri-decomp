/*
 * enemy/em027_prog.cpp - enemy 027's program: the `em027_prog_tbl` entry points (reset, init, frame update, action
 *   dispatch, event hook, motion events, state query, event request, area start, seated clear, alert test, the
 *   mini-work init/step), the per-action state steps the `state_sub` dispatchers select, the two sound programs and
 *   the demo (cut-scene) steps.
 * RANGE. .text 0x801AA154-0x801B0010 (71 functions); extab 0x8000F374-0x8000F54C, extabindex 0x8002AC18-0x8002AEDC,
 *   .rodata 0x805701A0-0x80570260 (three turn sequences), .data 0x805B06D0-0x805B0FCC (`em027_prog_tbl` first, then
 *   the dispatchers' switch tables and the sound programs `enemy/em_se_record.h` describes), .sdata
 *   0x80791A88-0x80791A90 (`em027_se_loop`), .sdata2 0x80798910-0x80798B38 (the float pool, from literals).
 * FLAGS. `cflags_main` with `#pragma peephole off` and `#pragma fp_contract off` file-wide (the flags of the twin
 *   `enemy/em029_prog.cpp`; retail keeps `fmuls`+`fadds` in `em027_act13_7`).
 * NAMES. The file name follows the runtime dump's `em027_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em027ProgTbl` slot each fills, after `Em012ProgTbl`/`Em022ProgTbl`): em027_reset, em027_init,
 *   em027_frame_update, em027_action_dispatch, em027_event_notify, em027_motion_events, em027_state_ck,
 *   em027_event_request, em027_area_start_set, em027_seated_clear, em027_alert_ck, em027_mini_init, em027_mini_step.
 *   GUESS (dispatcher slot): em027_act<action>_<first sub-state> and em027_act<action>_dispatch, em027_act10_se_play,
 *   em027_act11_se_play; GUESS (from the bodies): em027_area_target_set, em027_area_pos_set,
 *   em027_ground_variant_roll, em027_effect_spawn.
 *   GUESS (data): em027_turn_seq_0..2, em027_se_loop, em027_se_tbl_mot<N> / em027_se_rec_mot<N>_<x> (the motion that
 *   plays them), em027_se_tbl_/_rec_/_branch_act11_<n>_<x>, em027_se_rec_unused_0..4 (records no entry points at);
 *   `_ENEMY_WORK::em027_0x328.seated_0x328`, `resume_mode_0xB04`, `seated_keep_0xB05` and their copies in the
 *   area record (`_ENEMY_MINI_WORK::resume_mode_0x34`/`seated_keep_0x35`).
 * RESIDUALS. 1 row short of the target, every data section byte-identical (the `.sdata2` object ends 4 bytes short
 *   of the claim's alignment pad): `em027_init` - retail keeps an unreachable `b` to the switch end after each
 *   variant switch's dispatch (the empty `case 0:` block), ours drops both (8 B); an empty statement, a block, a
 *   `*= 1.0f` and an `if (0)` in that case measured the same.
 *   flipcheck: the callee `em_area_engaged_ck` (registry proposal for 0x801B0010, `enemy/em030_prog.cpp`) is not
 *   a map name yet, so a flip links `undefined` until that rename lands.
 * SHAPES. A case-level `return` at the end of a state switch costs a `b`: the steps `break`.  One-case signed
 *   compares are one-case switches (`em027_event_notify`).  `em_wave_segment`, `eft_slot_marks_set_sync` and `ran_suu`
 *   (in `em027_mini_step`) are called through `_c1` views whose argument order and widths give retail's code.
 *   The sound programs are defined after `em027_act11_se_play` (declared `extern` above) so the dispatchers load each table by name and the
 *   switch tables keep their places around them; the unreferenced records are `__declspec(export)`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/EnemyData.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/em_area_engaged_ck.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_8011D448.h"
#include "enemy/em_area_next.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"
#include "enemy/em_ground_rec_clear.h"
#include "ef/eft_slot.h"
#include "ef/eft_slot_shape_block_get.h"
#include "ef/eft009.h"
#include "ef/eft013_em_place.h"
#include "ef/fn_8010D1A8.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_event_request.h"
#include "stage/stg_w.h"
#include "unsplit/enemy.h"

#pragma peephole off
#pragma fp_contract off

/* `eft_slot_marks_set_sync` with the (entry, mode, area) arguments retail passes (`ef/eft_slot.h` spells two):
 * a cast call is the same direct call. */
/* `em_wave_segment` with the radius ahead of the two integers (retail evaluates it there). */
#define em_wave_segment_c1 ((void (*)(nw4r::math::VEC3*, nw4r::math::VEC3*, nw4r::math::VEC3*, f32, u32, u16))em_wave_segment)
/* `ran_suu` read at the full register width (the mini step narrows each use itself). */
#define ran_suu_c1 ((s32 (*)(s32))ran_suu)
#define eft_slot_marks_set_sync_c1 ((void (*)(struct EftSlot*, u32, u8))eft_slot_marks_set_sync)

/* The em027 program table (`em027_prog_tbl`): the entry points the shared enemy program driver calls.  Slot names
 * follow `Em012ProgTbl` and `Em022ProgTbl` (GUESS).
 * size: 0x6C */
struct Em027ProgTbl {
    /* +0x00 */ void (*reset_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ u32 pad_0x10;
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 action, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C[2];
    /* +0x24 */ u8 (*state_ck_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ s32 (*event_request_0x28)(void);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, s8* action, s8* sub);
    /* +0x34 */ void (*seated_clear_0x34)(struct _ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[2];
    /* +0x40 */ s32 (*alert_ck_0x40)(struct _ENEMY_WORK* self);
    /* +0x44 */ u32 pad_0x44[6];
    /* +0x5C */ void (*mini_init_0x5C)(_ENEMY_MINI_WORK* mini, u8 mode);
    /* +0x60 */ void (*mini_step_0x60)(_ENEMY_MINI_WORK* mini);
    /* +0x64 */ u32 pad_0x64[2];
};

extern "C" {

void em027_area_target_set(struct _ENEMY_WORK* self);
void em027_area_pos_set(struct _ENEMY_WORK* self, _ENEMY_DATA* data);
s32 em027_ground_variant_roll(u16 id);
void em027_reset(struct _ENEMY_WORK* self);
void em027_init(struct _ENEMY_WORK* self, u8 mode);
void em027_event_notify(struct _ENEMY_WORK* self, u8 action, u8 sub);
void em027_frame_update(struct _ENEMY_WORK* self);
void em027_act0_0(struct _ENEMY_WORK* self);
void em027_act0_dispatch(struct _ENEMY_WORK* self);
void em027_act1_0(struct _ENEMY_WORK* self);
void em027_act1_1(struct _ENEMY_WORK* self);
void em027_act1_2(struct _ENEMY_WORK* self);
void em027_act1_3(struct _ENEMY_WORK* self);
void em027_act1_4(struct _ENEMY_WORK* self, u8 kind);
void em027_act1_5(struct _ENEMY_WORK* self, u8 kind);
void em027_act1_6(struct _ENEMY_WORK* self);
void em027_act1_7(struct _ENEMY_WORK* self);
void em027_act1_8(struct _ENEMY_WORK* self);
void em027_act1_9(struct _ENEMY_WORK* self);
void em027_act1_dispatch(struct _ENEMY_WORK* self);
void em027_act2_0(struct _ENEMY_WORK* self, f32 blend);
void em027_act2_1(struct _ENEMY_WORK* self, f32 blend);
void em027_act2_2(struct _ENEMY_WORK* self, u8 kind);
void em027_act2_3(struct _ENEMY_WORK* self, f32 arg1);
void em027_act2_4(struct _ENEMY_WORK* self);
void em027_act2_5(struct _ENEMY_WORK* self, u8 kind);
void em027_act2_8(struct _ENEMY_WORK* self);
void em027_act2_10(struct _ENEMY_WORK* self);
void em027_act2_11(struct _ENEMY_WORK* self);
void em027_act2_12(struct _ENEMY_WORK* self);
void em027_act2_13(struct _ENEMY_WORK* self, u8 kind);
void em027_act2_15(struct _ENEMY_WORK* self);
void em027_act2_16(struct _ENEMY_WORK* self, f32 blend);
void em027_act2_18(struct _ENEMY_WORK* self);
void em027_act2_20(struct _ENEMY_WORK* self);
void em027_act2_dispatch(struct _ENEMY_WORK* self);
void em027_act7_0(struct _ENEMY_WORK* self);
void em027_act7_1(struct _ENEMY_WORK* self);
void em027_act7_2(struct _ENEMY_WORK* self);
void em027_act7_3(struct _ENEMY_WORK* self);
void em027_act7_dispatch(struct _ENEMY_WORK* self);
void em027_act10_se_play(struct _ENEMY_WORK* self);
void em027_act11_se_play(struct _ENEMY_WORK* self);
void em027_act12_0(struct _ENEMY_WORK* self);
void em027_act12_1(struct _ENEMY_WORK* self);
void em027_act12_2(struct _ENEMY_WORK* self);
void em027_act12_dispatch(struct _ENEMY_WORK* self);
void em027_act13_0(struct _ENEMY_WORK* self);
void em027_act13_1(struct _ENEMY_WORK* self);
void em027_act13_2(struct _ENEMY_WORK* self);
void em027_act13_3(struct _ENEMY_WORK* self);
void em027_act13_4(struct _ENEMY_WORK* self);
void em027_act13_5(struct _ENEMY_WORK* self);
void em027_act13_6(struct _ENEMY_WORK* self);
void em027_act13_7(struct _ENEMY_WORK* self);
void em027_act13_8(struct _ENEMY_WORK* self);
void em027_act13_9(struct _ENEMY_WORK* self);
void em027_act13_10(struct _ENEMY_WORK* self);
void em027_act13_11(struct _ENEMY_WORK* self);
void em027_act13_12(struct _ENEMY_WORK* self);
void em027_act13_dispatch(struct _ENEMY_WORK* self);
s32 em027_alert_ck(struct _ENEMY_WORK* self);
void em027_action_dispatch(struct _ENEMY_WORK* self);
void em027_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, s32 joint, s32 delta, f32 scale);
void em027_motion_events(struct _ENEMY_WORK* self);
u8 em027_state_ck(struct _ENEMY_WORK* self, u8 kind);
s32 em027_event_request(void);
void em027_area_start_set(struct _ENEMY_WORK* self, s8* action, s8* sub);
void em027_seated_clear(struct _ENEMY_WORK* self);
void em027_mini_init(_ENEMY_MINI_WORK* mini, u8 mode);
void em027_mini_step(_ENEMY_MINI_WORK* mini);

extern EmSeEntry em027_se_tbl_mot1[];
extern EmSeEntry em027_se_tbl_mot2[];
extern EmSeEntry em027_se_tbl_mot31[];
extern EmSeEntry em027_se_tbl_mot32[];
extern EmSeEntry em027_se_tbl_mot122[];
extern EmSeEntry em027_se_tbl_mot123[];
extern EmSeEntry em027_se_tbl_mot159[];
extern EmSeEntry em027_se_tbl_mot124[];
extern EmSeEntry em027_se_tbl_mot141[];
extern EmSeEntry em027_se_tbl_mot168[];
extern EmSeEntry em027_se_tbl_mot125[];
extern EmSeEntry em027_se_tbl_mot142[];
extern EmSeEntry em027_se_tbl_mot212[];
extern EmSeEntry em027_se_tbl_mot213[];
extern EmSeEntry em027_se_tbl_mot248[];
extern EmSeEntry em027_se_tbl_act11_1[];
extern EmSeEntry em027_se_tbl_act11_2[];
extern EmSeEntry em027_se_tbl_act11_59[];

Em027ProgTbl em027_prog_tbl = {
    em027_reset,
    em027_init,
    em027_frame_update,
    em027_action_dispatch,
    0,
    em027_event_notify,
    em027_motion_events,
    {0, 0},
    em027_state_ck,
    em027_event_request,
    0,
    em027_area_start_set,
    em027_seated_clear,
    {0, 0},
    em027_alert_ck,
    {0, 0, 0, 0, 0, 0},
    em027_mini_init,
    em027_mini_step,
    {0, 0},
};

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head, then three
 * four-word rows whose first word carries the motion in its high half. */
const u32 em027_turn_seq_0[16] = {
    0x0000002A, 0x000000AA, 0x0000001E, 0x40000000,
    0x00010000, 0x00000004, 0x00000000, 0x00000000,
    0x00050000, 0x00000008, 0x00000000, 0x00000001,
    0x00060000, 0x00000008, 0x00000000, 0x00000001,
};

const u32 em027_turn_seq_1[16] = {
    0x0000000E, 0x0000014C, 0x00000000, 0x2AAB0000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
};

const u32 em027_turn_seq_2[16] = {
    0x00000006, 0x0000007A, 0x00000000, 0x80000000,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000F0000, 0x00000004, 0x00000000, 0x00000000,
    0x00100000, 0x00000004, 0x00000000, 0x00000000,
};

/* The loop word the two `0xFE` sound entries point at (`.sdata`). */
u32 em027_se_loop[2] = {0x01060000, 0x00000000};

void em027_area_target_set(struct _ENEMY_WORK* self) {
    u32 reset;

    reset = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 1100.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 4, 1100.0f);
                break;
            default:
                em_move_target_set(self, 3, 4, 1100.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 1200.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1200.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 1500.0f);
                break;
            case 2:
                em_move_target_set(self, 3, 5, 1900.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1500.0f);
                break;
            }
            break;
        default:
            reset = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 1180.0f);
                break;
            case 4:
                em_move_target_set(self, 3, 5, 1200.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1180.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 760.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 900.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 760.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 1300.0f);
                break;
            case 4:
                em_move_target_set(self, 3, 5, 1300.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1300.0f);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 1200.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 1000.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1200.0f);
                break;
            }
            break;
        default:
            reset = 1;
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 5:
                em_move_target_set(self, 2, 4, 700.0f);
                break;
            case 7:
                em_move_target_set(self, 3, 5, 700.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 700.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 5:
                em_move_target_set(self, 2, 6, 800.0f);
                break;
            case 7:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 1000.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1000.0f);
                break;
            }
            break;
        case 7:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            }
            break;
        default:
            reset = 1;
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 4, 0, 1100.0f);
                break;
            case 3:
                em_move_target_set(self, 5, 1, 1200.0f);
                break;
            default:
                em_move_target_set(self, 4, 0, 1100.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 4, 0, 1200.0f);
                break;
            case 3:
                em_move_target_set(self, 5, 1, 1100.0f);
                break;
            default:
                em_move_target_set(self, 4, 0, 1200.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 4, 0, 1100.0f);
                break;
            case 2:
                em_move_target_set(self, 5, 1, 1300.0f);
                break;
            default:
                em_move_target_set(self, 4, 0, 1100.0f);
                break;
            }
            break;
        default:
            reset = 1;
            break;
        }
        break;
    case 5:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            case 6:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 900.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 1000.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 900.0f);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 2:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            case 6:
                em_move_target_set(self, 3, 5, 800.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 800.0f);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 1:
                em_move_target_set(self, 2, 4, 1000.0f);
                break;
            case 3:
                em_move_target_set(self, 3, 5, 900.0f);
                break;
            default:
                em_move_target_set(self, 2, 4, 1000.0f);
                break;
            }
            break;
        default:
            reset = 1;
            break;
        }
        break;
    default:
        reset = 1;
        break;
    }
    if (reset == 1) {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        em_reaction_req(self, 5, 2, 0xA);
    }
}

void em027_area_pos_set(struct _ENEMY_WORK* self, _ENEMY_DATA* data) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    u32 next;
    u8 area;
    u8 data_area;
    u8 index;
    EftShapeBlockView* shape;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    index = 0;
    data_area = data->field_0x14;
    area = self->area_no;
    if (area == data_area) {
        index = data->field_0x36;
    } else {
        next = em_area_next(self, area, data_area);
        switch (stage_map_kind_get(self->field_0x1E0)) {
        case 1:
            switch (self->area_no) {
            case 1:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 3:
                    index = 0;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 2:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 3:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 2:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            }
            break;
        case 2:
            switch (self->area_no) {
            case 1:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 4:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 2:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 3:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 4:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 4:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            }
            break;
        case 3:
            switch (self->area_no) {
            case 1:
                switch ((u8)next) {
                case 5:
                    index = 0;
                    break;
                case 7:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 3:
                switch ((u8)next) {
                case 5:
                    index = 0;
                    break;
                case 7:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 5:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 7:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            }
            break;
        case 4:
            switch (self->area_no) {
            case 1:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 2:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 3:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 2:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            }
            break;
        case 5:
            switch (self->area_no) {
            case 1:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 6:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 2:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 3:
                switch ((u8)next) {
                case 2:
                    index = 0;
                    break;
                case 6:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            case 6:
                switch ((u8)next) {
                case 1:
                    index = 0;
                    break;
                case 3:
                    index = 1;
                    break;
                default:
                    index = 0;
                    break;
                }
                break;
            }
            break;
        }
    }
    shape = eft_slot_shape_block_get((struct EftSlot*)data, self->area_no, index);
    if (shape != NULL) {
        if (shape->kind_0x00 == 0) {
            setVector3(&a, shape->a_0x04.x, shape->a_0x04.y, shape->a_0x04.z);
            em_wave_amp(&self->pos, &a, shape->radius_0x1C, 0, self->bits_0x1EC);
        } else {
            setVector3(&a, shape->a_0x04.x, shape->a_0x04.y, shape->a_0x04.z);
            setVector3(&b, shape->b_0x10.x, shape->b_0x10.y, shape->b_0x10.z);
            em_wave_segment_c1(&self->pos, &a, &b, shape->radius_0x1C, 0, self->bits_0x1EC);
        }
        self->field_0x1BC = 0;
        self->field_0x1C0 = self->bits_0x1EC;
        self->field_0x1C4 = 0;
        return;
    }
    self->pos.z = 0.0f;
    self->pos.y = 0.0f;
    self->pos.x = 0.0f;
    self->field_0x1BC = 0;
    self->field_0x1C0 = self->bits_0x1EC;
    self->field_0x1C4 = 0;
    em_reaction_req(self, 5, 2, 0xA);
}

s32 em027_ground_variant_roll(u16 id) {
    EmGroundRec rec;
    s32 pct;

    em_ground_rec_clear(&rec);
    if (move_work_state_ck() == 0) {
        return 0;
    }
    if (em_ground_rec_find(id, &rec) == 0) {
        return 0;
    }
    if (rec.field_0x04_b == 1) {
        return 0;
    }
    pct = (u16)ran_suu(0) % 100;
    switch (rec.field_0x03) {
    case 3:
        return (pct < 60) + 1;
    case 0:
        return (pct < 10) + 1;
    default:
        return 0;
    }
}

/* r3 the work record; clears the first byte of its effect-slot block. */
void em027_reset(struct _ENEMY_WORK* self) {
    self->em027_0x328.seated_0x328 = 0;
}

void em027_init(struct _ENEMY_WORK* self, u8 mode) {
    EmGroundRec rec;
    _ENEMY_DATA* data;
    s8 variant;

    em_ground_rec_clear(&rec);
    if ((s32) (self->field_0x1C8 & 8) != 0) {
        em_status_bits_set(self, 0x80);
    }
    data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
    switch (mode) {
    case 0:
        if (self->field_0x00F == 0) {
            variant = em027_ground_variant_roll(self->field_0x01A);
            if (variant == 1) {
                self->field_0x00A = 0;
            } else if (variant == 2) {
                self->field_0x00A = 3;
            }
        }
        if ((em_ground_rec_find(self->field_0x01A, &rec) != 1) || (rec.area_0x01 != self->area_no)) {
            if (data != NULL) {
                em027_area_pos_set(self, data);
            } else {
                self->pos.z = 0.0f;
                self->pos.y = 0.0f;
                self->pos.x = 0.0f;
                self->field_0x1C4 = 0;
                self->field_0x1C0 = 0;
                self->field_0x1BC = 0;
            }
        }
        switch (self->resume_mode_0xB04) {
        case 3:
            em_motion_kind_set(self, 2, 1);
            break;
        case 2:
            em_motion_kind_set(self, 1, 1);
            break;
        }
        self->em027_0x328.seated_0x328 = self->seated_keep_0xB05;
        break;
    case 3:
        em027_area_target_set(self);
        switch (self->resume_mode_0xB04) {
        case 1:
            em_act_arm_unless_down(self, 2, 0);
            break;
        case 3:
            em_act_arm_unless_down(self, 2, 0x12);
            break;
        default:
            em_act_arm_unless_down(self, 2, 1);
            break;
        }
        em_state_refresh(self);
        break;
    case 1:
    case 4:
        variant = em027_ground_variant_roll(self->field_0x01A);
        if (variant == 1) {
            self->field_0x00A = 0;
        } else if (variant == 2) {
            self->field_0x00A = 3;
        }
        break;
    }
    switch (self->team) {
    case 27:
        switch (self->field_0x00A) {
        case 0:
            break;
        case 1:
            self->field_0x1CC *= 0.8f;
            break;
        case 2:
            self->field_0x1CC *= 0.5f;
            em_max_hp_set(self, 0x50);
            break;
        case 3:
            self->field_0x1CC *= 1.5f;
            em_max_hp_set(self, 0xC8);
            break;
        default:
            self->field_0x00A = 0;
            break;
        }
        break;
    case 28:
        switch (self->field_0x00A) {
        case 0:
            break;
        case 1:
            self->field_0x1CC *= 0.8f;
            break;
        case 2:
            self->field_0x1CC *= 0.5f;
            em_max_hp_set(self, 0x5F);
            break;
        case 3:
            self->field_0x1CC *= 1.5f;
            em_max_hp_set(self, 0x96);
            break;
        default:
            self->field_0x00A = 0;
            break;
        }
        break;
    }
    if ((data != NULL) && (self->area_no == data->field_0x14)) {
        copyVec3(&self->aim, &data->vec_0x24);
    }
    self->resume_mode_0xB04 = 0;
}

void em027_event_notify(struct _ENEMY_WORK* self, u8 action, u8 sub) {
    switch (action) {
    case 10:
        switch (sub) {
        case 0xF8:
            em_demo_pos_set(self, 57.4f, 4.0f, -169.9f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
            break;
        }
        break;
    case 11:
        switch (sub) {
        case 0x3B:
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 2:
                    em_demo_pos_set(self, -5256.0f, 0.0f, -446.0f);
                    em_demo_rot_set(self, 0.0f, 85.0f, 0.0f);
                    break;
                }
                break;
            case 4:
                switch (self->area_no) {
                case 3:
                    em_demo_pos_set(self, 539.2f, 7.0f, -6514.2f);
                    em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
                    break;
                }
                break;
            }
            break;
        }
        break;
    }
}

void em027_frame_update(struct _ENEMY_WORK* self) {
    u16 roll;
    _ENEMY_DATA* data;
    u8 hit;

    roll = ran_suu(1);
    if ((em_die_ck(self) == 0) && ((s32) (self->field_0x1C8 & 8) != 0) && (em_busy_ck(self) == 1) && (em_act_ck(self, 0xD, 0) == 0) && (em_area_engaged_ck(self->area_no) == 1)) {
        em_hit_by_set(self, 0, 1);
        return;
    }
    data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
    if (data != NULL) {
        if (self->area_no == data->field_0x14) {
            copyVec3(&self->aim, &data->vec_0x24);
        }
        switch (data->field_0x08) {
        case 0:
        case 2:
        case 5:
            if ((em_die_ck(self) == 0) && (self->area_no == data->field_0x14)) {
                em_motion_kind_set(self, 0, 0);
            }
            if (self->field_0x833 == 1) {
                if ((em_die_ck(self) == 0) && (self->area_no == data->field_0x14)) {
                    em_motion_kind_set(self, 1, 1);
                }
                if ((self->team == 0x1C) && (self->field_0x00A == 2)) {
                    eft_slot_marks_set_sync_c1((struct EftSlot*)data, 2, self->area_no);
                } else {
                    eft_slot_marks_set_sync_c1((struct EftSlot*)data, 1, self->area_no);
                }
            }
            if (em_die_ck(self) == 0) {
                if ((data->field_0x0B == 2) && (self->area_no != data->field_0x14) && (self->field_0x43B == 0)) {
                    em_motion_kind_set(self, 1, 1);
                    em_hit_by_set(self, (s16) ((roll & 0x7F) + 0x1E), 0);
                }
                if ((data->field_0x08 == 5) && (data->field_0x0B == 0) && (self->area_no == data->field_0x14)) {
                    self->em027_0x328.seated_0x328 = 1;
                    self->seated_keep_0xB05 = 1;
                    return;
                }
            }
            break;
        case 4:
            if ((em_die_ck(self) == 0) && (self->area_no == data->field_0x14)) {
                if ((self->field_0x833 == 1) || (em_get_mot_no(self) == 9)) {
                    em_motion_kind_set(self, 1, 1);
                }
                hit = self->field_0x43B;
                if ((hit != 1) && (hit != 2) && ((s16) data->field_0x22 == 1)) {
                    if (self->seated_keep_0xB05 == 1) {
                        em_motion_kind_set(self, 2, 1);
                        em_hit_by_set(self, (s16) ((roll & 0x7F) + 0x1E), 0);
                    } else {
                        em_motion_kind_set(self, 1, 0);
                        em_hit_by_set(self, (s16) ((roll & 0x7F) + 0x1E), 0);
                    }
                }
                if (self->field_0x43B == 2) {
                    self->resume_mode_0xB04 = 1;
                    return;
                }
            }
            break;
        case 3:
            if ((em_die_ck(self) == 0) && (self->area_no == data->field_0x14)) {
                if (self->field_0x00A == 0) {
                    if ((data->field_0x0F == 3) && (data->field_0x10 != 0xFF)) {
                        em_motion_kind_set(self, 1, 1);
                    } else {
                        if (self->field_0x43C == 0) {
                            em_hit_by_set(self, (s16) ((roll & 0x7F) + 0x1E), 0);
                        }
                        em_motion_kind_set(self, 2, 1);
                    }
                } else {
                    if (self->field_0x43C == 0) {
                        em_hit_by_set(self, (s16) ((roll & 0x7F) + 0x1E), 0);
                    }
                    em_motion_kind_set(self, 2, 1);
                }
                self->resume_mode_0xB04 = 1;
                return;
            }
            break;
        }
    } else if (em_die_ck(self) == 0) {
        if (self->field_0x833 == 1) {
            em_motion_kind_set(self, 1, 0);
        }
        if ((s16) self->field_0x440 > 0xB4) {
            em_motion_kind_set(self, 0, 0);
        }
    }
}

/* r3 the work record; the escape action's step machine (motion 1, then the end request). */
void em027_act0_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the sub-state dispatcher over the escape action's three recorded sub-steps. */
void em027_act0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act0_0(self);
        return;
    case 1:
        em027_act0_0(self);
        return;
    case 2:
        em027_act0_0(self);
        return;
    }
}

/* r3 the work record; the two-motion escape variant (0x17 then 0x18). */
void em027_act1_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 0x14, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0x14, 0);
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the three-motion escape variant (0x15, motion 2, 0x18). */
void em027_act1_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0x14, 0);
            break;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the single-motion escape variant (0x16). */
void em027_act1_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 0x14, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x19). */
void em027_act1_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the variant selector; the two-motion escape whose second motion depends on
 * the selector (7 or 0x1D). */
void em027_act1_4(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            em_mot_set(self, 7, 0x10, 0);
            break;
        case 1:
            em_mot_set(self, 0x1D, 0x10, 0);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record, r4 the selector the escape's first motion depends on; the variant that also
 * arms the effect scale. */
void em027_act1_5(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            em_mot_set(self, 8, 0xA, 0);
            break;
        case 1:
            em_mot_set(self, 0x1A, 0x10, 0);
            break;
        case 2:
            em_mot_set(self, 0x1C, 0x10, 0);
            break;
        }
        em_mot_speed_set(self, 1.3f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x1B). */
void em027_act1_6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0xD). */
void em027_act1_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x11). */
void em027_act1_8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-motion escape variant (0xA, 0xB, 0xC). */
void em027_act1_9(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 0, 0);
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xC, 0, 0);
            break;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the escape band's sub-state dispatcher (the 13 entries the `.data` table at
 * 0x805B073C selects). */
void em027_act1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act1_0(self);
        return;
    case 1:
        em027_act1_1(self);
        return;
    case 2:
        em027_act1_2(self);
        return;
    case 3:
        em027_act1_3(self);
        return;
    case 4:
        em027_act1_4(self, 0);
        return;
    case 5:
        em027_act1_5(self, 0);
        return;
    case 6:
        em027_act1_6(self);
        return;
    case 7:
        em027_act1_7(self);
        return;
    case 8:
        em027_act1_8(self);
        return;
    case 9:
        em027_act1_9(self);
        return;
    case 10:
        em027_act1_5(self, 1);
        return;
    case 11:
        em027_act1_5(self, 2);
        return;
    case 12:
        em027_act1_4(self, 1);
        return;
    }
}

/* r3 the work record, f1 the blend the motion's effect is armed with; the held-swing step
 * (motion 3, mask 0x80). */
void em027_act2_0(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, 1.6f);
        }
        em_approach_start(self, blend, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, f1 the blend; the held-swing step (motion 4, mask 0x80). */
void em027_act2_1(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, 1.5f);
        }
        em_approach_start(self, blend, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the selector; the selector-driven escape variant (the sub-state dispatches
 * 0 or 1 to the position/rotation reset and the rest to be begun). */
void em027_act2_2(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            em_turn_seq_start(self, (void*)em027_turn_seq_0, 0, 0, 0);
            break;
        case 1:
            em_turn_seq_start(self, (void*)em027_turn_seq_0, 1, 0, 0);
            break;
        case 2:
            em_turn_seq_start(self, (void*)em027_turn_seq_0, 0, 0, 0);
            em_mot_speed_set(self, 1.5f);
            break;
        }
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em027_turn_seq_0) == 1) {
            em_action_finish(self);
            break;
        }
        if (kind == 2) {
            em_mot_speed_set(self, 1.5f);
        }
        break;
    }
}

/* r3 the work record, r4 the escape's second-motion selector; the burst step. */
void em027_act2_3(struct _ENEMY_WORK* self, f32 arg1) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 9, 0, 0);
            break;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0x48);
            em_approach_start(self, arg1, 0);
            break;
        }
        break;
    case 3:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the "effect shot" step (one table entry, then the end request). */
void em027_act2_4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em027_turn_seq_1, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em027_turn_seq_1) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the table variant (0, 1 or anything else = 0x4000); the "effect shot"
 * variant step. */
void em027_act2_5(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (kind == 0) {
            em_turn_seq_start(self, (void*)em027_turn_seq_2, 0, 0, 0);
            return;
        }
        if (kind == 1) {
            em_turn_seq_start(self, (void*)em027_turn_seq_2, 1, 0, 0);
            return;
        }
        em_turn_seq_start(self, (void*)em027_turn_seq_2, 0, 0, 0x4000);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em027_turn_seq_2) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the timer-gated step: arms the motion, holds for 0x100 + the low byte of the
 * action bit field, then requests the end. */
void em027_act2_8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0xFF) + 0x100;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        em_turn_to_target(self, 0x80);
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the swing step (motion 4, then a 0x1E + the action bit field hold). */
void em027_act2_10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, 1.5f);
        }
        em_approach_start(self, 0.0f, 0x10);
        self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x1E;
        return;
    case 1:
        em_approach_step(self, 0, 0x80);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the instant swing step (motion 3). */
void em027_act2_11(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, 1.6f);
        }
        em_approach_start(self, 0.0f, 0x10);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the shot step (motion 4, effect scale, 0x1E + the action bit field). */
void em027_act2_12(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, 1.2f);
        self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
        return;
    case 1:
        em_turn_to_target(self, 0x100);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the variant selector; the held shot step. */
void em027_act2_13(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        switch (kind) {
        case 0:
            em_mot_speed_set(self, 1.2f);
            self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
            break;
        case 1:
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x1E;
            break;
        }
        em_approach_start(self, 0.0f, 0x10);
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->state_0x006 == 0 && em_approach_step(self, 0, 0x100) == 1) {
            self->state_0x006++;
        }
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the motion-frame-gated step that arms the second motion and then holds it for
 * 0x1E + the low six bits of the action bit field. */
void em027_act2_15(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        em_turn_to_target(self, 0x100);
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            em_mot_speed_set(self, 1.2f);
            self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
            break;
        }
        break;
    case 2:
        em_turn_to_target(self, 0x100);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record, f1 the blend; the held-swing step (motion 4, effect scale, mask 0x180). */
void em027_act2_16(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, 1.2f);
        em_approach_start(self, blend, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x180) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the motion-frame-gated burst step (the attack ended when the window opens). */
void em027_act2_18(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        em_turn_to_target(self, 0x100);
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, 1.5f);
            }
            em_approach_start(self, 0.0f, 0);
            break;
        }
        break;
    case 2:
        if (em_get_mot_no(self) == 4) {
            if (self->state_0x006 < 1U && em_mot_end_ck(self) == 1) {
                if ((self->bits_0x1EC & 0xF) < 8 || self->timer_0x020 > 4) {
                    em_mot_set(self, 0x1B, 4, 0);
                    self->state_0x006++;
                }
                self->timer_0x020 = self->timer_0x020 + 1;
            }
        } else if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, 1.5f);
            }
        }
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the motion-frame-gated burst step (the short variant). */
void em027_act2_20(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        em_turn_to_target(self, 0x100);
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, 1.5f);
            }
            em_approach_start(self, 0.0f, 0);
            break;
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* r3 the work record; the shot band's sub-state dispatcher (the 21 entries the `.data` table at
 * 0x805B0720 selects). */
void em027_act2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act2_0(self, -200.0f);
        return;
    case 1:
        em027_act2_1(self, -200.0f);
        return;
    case 2:
        em027_act2_2(self, 0);
        return;
    case 3:
        em027_act2_3(self, -200.0f);
        return;
    case 4:
        em027_act2_4(self);
        return;
    case 5:
        em027_act2_5(self, 0);
        return;
    case 6:
        em027_act2_5(self, 1);
        return;
    case 7:
        em027_act2_5(self, 2);
        return;
    case 8:
        em027_act2_8(self);
        return;
    case 9:
        em027_act2_1(self, -400.0f);
        return;
    case 10:
        em027_act2_10(self);
        return;
    case 11:
        em027_act2_11(self);
        return;
    case 12:
        em027_act2_12(self);
        return;
    case 13:
        em027_act2_13(self, 0);
        return;
    case 14:
        em027_act2_13(self, 1);
        return;
    case 15:
        em027_act2_15(self);
        return;
    case 16:
        em027_act2_16(self, -400.0f);
        return;
    case 17:
        em027_act2_2(self, 1);
        return;
    case 18:
        em027_act2_18(self);
        return;
    case 19:
        em027_act2_2(self, 2);
        return;
    case 20:
        em027_act2_20(self);
        return;
    }
}

/* r3 the work record; the two-motion step whose second motion depends on the enemy team. */
void em027_act7_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        if (self->team == 0x1C) {
            em_hit_window_set_default(self, 0, 4);
            return;
        }
        em_hit_window_set_default(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x12) with the part-2 arming. */
void em027_act7_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0x10, 0);
        em_hit_window_set_default(self, 0, 2);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the part-3 arming step (0x14) that searches while its window is closed. */
void em027_act7_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_hit_window_set_default(self, 0, 3);
        return;
    case 1:
        if (em_frame_check(self, 1, 130.0f, 0.0f) == 0) {
            self->field_0x1C0 = self->field_0x1C0 + 0x17E;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the part-5 arming step (0xCE) with the effect scale and the 0x200 mask. */
void em027_act7_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCE, 6, 0);
        em_mot_speed_set(self, 0.6f);
        em_hit_window_set_default(self, 0, 5);
        return;
    case 1:
        if (em_frame_check(self, 3, 28.0f, 68.0f) == 1) {
            em_turn_to_target(self, 0x200);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the four-entry part-armer dispatcher. */
void em027_act7_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act7_0(self);
        return;
    case 1:
        em027_act7_1(self);
        return;
    case 2:
        em027_act7_2(self);
        return;
    case 3:
        em027_act7_3(self);
        return;
    }
}

void em027_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        em_se_tbl_play(self, em027_se_tbl_mot1, 0, 1);
        break;
    case 2:
        em_se_tbl_play(self, em027_se_tbl_mot2, 0, 2);
        break;
    case 31:
        em_se_tbl_play(self, em027_se_tbl_mot31, 0, 31);
        break;
    case 32:
        em_se_tbl_play(self, em027_se_tbl_mot32, 0, 32);
        break;
    case 122:
        em_se_tbl_play(self, em027_se_tbl_mot122, 0, 122);
        break;
    case 123:
        em_se_tbl_play(self, em027_se_tbl_mot123, 0, 123);
        break;
    case 159:
        em_se_tbl_play(self, em027_se_tbl_mot159, 0, 159);
        break;
    case 160:
        em_se_tbl_play(self, em027_se_tbl_mot159, 0, 160);
        break;
    case 124:
        em_se_tbl_play(self, em027_se_tbl_mot124, 0, 124);
        break;
    case 141:
        em_se_tbl_play(self, em027_se_tbl_mot141, 0, 141);
        break;
    case 168:
        em_se_tbl_play(self, em027_se_tbl_mot168, 0, 168);
        break;
    case 125:
        em_se_tbl_play(self, em027_se_tbl_mot125, 0, 125);
        break;
    case 142:
        em_se_tbl_play(self, em027_se_tbl_mot142, 0, 142);
        break;
    case 212:
        em_se_tbl_play(self, em027_se_tbl_mot212, 0, 212);
        break;
    case 213:
        em_se_tbl_play(self, em027_se_tbl_mot213, 0, 213);
        break;
    case 248:
        em_se_tbl_play(self, em027_se_tbl_mot248, 0, 248);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

void em027_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_1, 0, 1);
        break;
    case 2:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_2, 0, 2);
        break;
    case 17:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_1, 0, 17);
        break;
    case 18:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_2, 0, 18);
        break;
    case 59:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_59, 0, 59);
        break;
    default:
        em_se_tbl_play_alt(self, em027_se_tbl_act11_1, 0, 1);
        break;
    }
}

/* The sound-effect programs `em027_act10_se_play`/`em027_act11_se_play` pass to the table walkers. */
EmSeRecord em027_se_rec_mot1_a = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot1[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot1_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot2_a = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot2[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot2_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot31_a = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_mot31_b = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_mot31_c = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_mot31_d = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot31[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x00, {0, 0, 0}, &em027_se_rec_mot31_a},
    {0x00, {0, 0, 0}, &em027_se_rec_mot31_b},
    {0x00, {0, 0, 0}, &em027_se_rec_mot31_c},
    {0x00, {0, 0, 0}, &em027_se_rec_mot31_d},
    {0xFF, {0, 0, 0}, 0},
};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em027_se_rec_unused_0 = {100, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_mot32_a = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_mot32_b = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_mot32_c = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot32[4] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot32_a},
    {0x00, {0, 0, 0}, &em027_se_rec_mot32_b},
    {0x00, {0, 0, 0}, &em027_se_rec_mot32_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot122_a = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot122[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot122_a},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot123_a = {107, 0, 10, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot123[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot123_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot159_a = {105, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot159[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot159_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot124_a = {100, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_mot124_b = {108, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot124[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x18)},
    {0x00, {0, 0, 0}, &em027_se_rec_mot124_a},
    {0x00, {0, 0, 0}, &em027_se_rec_mot124_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot141_a = {100, 0, 6, 30, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot141[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot141_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot125_a = {26, 0, 8, 56, 0, {1, 8, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot125[4] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x98)},
    {0x02, {0, 0, 0}, &em027_se_rec_mot125_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot142_a = {26, 0, 20, 180, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot142[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot142_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot168_a = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot168[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot168_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em027_se_rec_mot212_a = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot212[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot212_a},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em027_se_loop},
};

EmSeRecord em027_se_rec_mot213_a = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot213[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot213_a},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)em027_se_loop},
};

EmSeRecord em027_se_rec_mot248_a = {107, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em027_se_tbl_mot248[2] = {
    {0x00, {0, 0, 0}, &em027_se_rec_mot248_a},
    {0xFE, {0, 0, 0}, 0},
};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em027_se_rec_unused_1 = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_a_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_e = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_b_a = {0, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_f = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_c_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_g = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_d_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_h = {104, 0, 10, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_1_i = {104, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

extern EmSeBranch em027_se_branch_act11_1_a;
extern EmSeBranch em027_se_branch_act11_1_b;
extern EmSeBranch em027_se_branch_act11_1_c;
extern EmSeBranch em027_se_branch_act11_1_d;
EmSeEntry em027_se_tbl_act11_1[14] = {
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_1_a)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_1_b)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_1_c)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_1_d)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_1_e},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_1_f},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_1_g},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_1_h},
    {0x0F, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_i},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em027_se_tbl_act11_1_a[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_a_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_1[7])},
};

EmSeEntry em027_se_tbl_act11_1_b[3] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_b_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_1[6])},
};

EmSeEntry em027_se_tbl_act11_1_c[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_c_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_1[7])},
};

EmSeEntry em027_se_tbl_act11_1_d[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_d_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_1[8])},
};

EmSeBranch em027_se_branch_act11_1_a = {0, 102, em027_se_tbl_act11_1_a};

EmSeBranch em027_se_branch_act11_1_b = {0, 101, em027_se_tbl_act11_1_b};

EmSeBranch em027_se_branch_act11_1_c = {0, 106, em027_se_tbl_act11_1_c};

EmSeBranch em027_se_branch_act11_1_d = {0, 103, em027_se_tbl_act11_1_d};

/* A record no table entry points at; kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em027_se_rec_unused_2 = {101, 0, 4, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_a_a = {0, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_a_b = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_b_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_e = {102, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_c_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_f = {103, 0, 8, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em027_se_rec_act11_2_d_a = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

/* Two records no table entry points at (the `0x5`/`0xF` tail of the second table reuses the first table's pair);
 * kept in the link (force-active in retail). */
__declspec(export) EmSeRecord em027_se_rec_unused_3 = {104, 0, 10, 0, 0, {0, 8, 0, 0}, 0};

__declspec(export) EmSeRecord em027_se_rec_unused_4 = {104, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

extern EmSeBranch em027_se_branch_act11_2_a;
extern EmSeBranch em027_se_branch_act11_2_b;
extern EmSeBranch em027_se_branch_act11_2_c;
extern EmSeBranch em027_se_branch_act11_2_d;
EmSeEntry em027_se_tbl_act11_2[12] = {
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_2_a)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_2_b)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_2_c)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_branch_act11_2_d)},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_2_e},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_2_f},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_1_h},
    {0x0F, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em027_se_rec_act11_1_i},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em027_se_tbl_act11_2_a[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC)},
    {0x01, {0, 0, 0}, &em027_se_rec_act11_2_a_a},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_2_a_b},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_2[5])},
};

EmSeEntry em027_se_tbl_act11_2_b[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_2_b_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_2[5])},
};

EmSeEntry em027_se_tbl_act11_2_c[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_2_c_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_2[5])},
};

EmSeEntry em027_se_tbl_act11_2_d[2] = {
    {0x01, {0, 0, 0}, &em027_se_rec_act11_2_d_a},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em027_se_tbl_act11_2[6])},
};

EmSeBranch em027_se_branch_act11_2_a = {0, 101, em027_se_tbl_act11_2_a};

EmSeBranch em027_se_branch_act11_2_b = {0, 106, em027_se_tbl_act11_2_b};

EmSeBranch em027_se_branch_act11_2_c = {0, 102, em027_se_tbl_act11_2_c};

EmSeBranch em027_se_branch_act11_2_d = {0, 103, em027_se_tbl_act11_2_d};

EmSeRecord em027_se_rec_act11_59_a = {104, 0, 0, 2, 0, {0, 8, 0, 0}, 0};

EmSeEntry em027_se_tbl_act11_59[4] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x2)},
    {0x00, {0, 0, 0}, &em027_se_rec_act11_59_a},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

/* r3 the work record; the two-name step whose countdown is 0x78 + the low seven bits of the action
 * bit field. */
void em027_act12_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0x7F) + 0x78;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the swing step with the 0x3C + the low seven bits of the action bit field
 * hold. */
void em027_act12_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0x7F) + 0x3C;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x1B). */
void em027_act12_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-entry step dispatcher. */
void em027_act12_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act12_0(self);
        return;
    case 1:
        em027_act12_1(self);
        return;
    case 2:
        em027_act12_2(self);
        return;
    }
}

/* r3 the work record; the step that hands over to the area table once the per-area selector is set. */
void em027_act13_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (em_area_engaged_ck(self->area_no) == 1) {
                em_state_set(self, 0xD, 0);
                return;
            }
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the long scripted step (motions 0x17/7/0x1B/0x66/0x68 driven by the frame
 * counter `em_demo_time_ck(threshold)`). */
void em027_act13_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 0, 0);
        em_demo_pos_set(self, -6315.0f, 0.0f, 281.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_demo_time_ck(0x12C) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0x74);
            em_demo_pos_set(self, -6328.0f, 0.0f, 164.0f);
        }
        return;
    case 2:
        if (em_demo_time_ck(0x1AE) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0);
        }
        return;
    case 3:
        if (em_demo_time_ck(0x320) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0x14);
            self->field_0x1C0 = 0x3C72;
            em_demo_pos_set(self, -6377.0f, 0.0f, -282.0f);
        }
        return;
    case 4:
        if (em_demo_time_ck(0x3B0) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
            em_demo_pos_set(self, -5428.0f, 0.0f, -137.0f);
        }
        return;
    case 5:
        if (em_demo_time_ck(0x3E8) == 1) {
            self->state++;
            em_mot_set(self, 0x68, 0, 0);
            self->field_0x1C0 = 0;
            em_demo_pos_set(self, -5256.0f, 0.0f, -446.0f);
        }
        return;
    case 6:
        if (em_frame_check(self, 1, 2.0f, 0.0f) == 1) {
            em_flag_0x115_set(self);
        }
        if (em_demo_time_ck(0x51E) == 1) {
            self->state++;
            em_mot_set(self, 0x68, 0, 0);
            em_demo_pos_set(self, -5256.0f, 0.0f, -446.0f);
        }
        return;
    case 7:
        if (em_frame_check(self, 1, 2.0f, 0.0f) == 1) {
            em_flag_0x115_set(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x68) with the three rotation words seeded. */
void em027_act13_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x68, 0, 0);
        em_demo_pos_set(self, -5256.0f, 0.0f, -446.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x3C72;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-motion step (0x18, 0x17, 8) with rotating the second rotation word
 * negative. */
void em027_act13_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 0, 0);
        em_demo_pos_set(self, -5165.0f, 0.0f, 1592.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xFFFF71C7;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_demo_time_ck(0xC8) == 1) {
            self->state++;
            em_mot_set(self, 0x17, 4, 0);
        }
        return;
    case 2:
        if (em_demo_time_ck(0x1AE) == 1) {
            self->state++;
            em_mot_set(self, 8, 0, 0);
            self->field_0x1C0 = 0xFFFF71C7;
            em_demo_pos_set(self, -5165.0f, 0.0f, 1592.0f);
        }
        return;
    case 3:
        if (em_demo_time_ck(0x320) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0);
            self->field_0x1C0 = 0xFFFF5B06;
            em_demo_pos_set(self, -5053.0f, 0.0f, 1864.0f);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x1B) with the negative second rotation word. */
void em027_act13_4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 0, 0);
        em_demo_pos_set(self, -5053.0f, 0.0f, 1864.0f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xFFFF5B06;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em027_act13_5(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 sp5C;
    nw4r::math::VEC3 sp50;
    nw4r::math::VEC3 sp44;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    EmAilmentFx* fx;

    VEC3_ctor(&offset);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 1591.0f, 4.0f, 22.7f);
        em_demo_rot_set(self, 0.0f, -76.1f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x96) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0);
            em_demo_pos_set(self, 1114.8f, 4.0f, -146.1f);
            em_demo_rot_set(self, 0.0f, -102.0f, 0.0f);
            break;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0x13C) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 1043.5f, 4.0f, -27.1f);
            em_demo_rot_set(self, 0.0f, -102.0f, 0.0f);
            break;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x1F6) == 1) {
            self->state++;
            em_mot_set(self, 8, 0, 0x20);
            em_demo_pos_set(self, -202.1f, 4.0f, -226.5f);
            em_demo_rot_set(self, 0.0f, -102.0f, 0.0f);
            break;
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x1C, 4, 0);
            em_demo_pos_set(self, 296.9f, 4.0f, -241.7f);
            em_demo_rot_set(self, 0.0f, -102.0f, 0.0f);
            break;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x35A) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
            em_mot_speed_set(self, 0.3f);
            em_demo_pos_set(self, -76.3f, 4.0f, -203.1f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
            break;
        }
        break;
    case 6:
        if ((system_w.field_0x0c & 0x1F) == 0x10) {
            fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
            if (fx != NULL) {
                copyVec3(&offset, setVec3(&sp5C, fx->x, fx->y, fx->z));
                eft013_em_place(self, &offset, fx->joint, fx->scale, 0xA);
            } else {
                copyVec3(&offset, setVec3(&sp50, 0.0f, 100.0f, 0.0f));
                eft013_em_place(self, &offset, 2, 2.0f, 0xA);
            }
        }
        if (em_frame_check(self, 1, 6.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 8);
            break;
        }
        break;
    case 7:
        if ((system_w.field_0x0c & 0x1F) == 0x10) {
            fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
            if (fx != NULL) {
                copyVec3(&offset, setVec3(&sp44, fx->x, fx->y, fx->z));
                eft013_em_place(self, &offset, fx->joint, fx->scale, 0xA);
            } else {
                copyVec3(&offset, setVec3(&sp38, 0.0f, 100.0f, 0.0f));
                eft013_em_place(self, &offset, 2, 2.0f, 0xA);
            }
        }
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0x34);
            em_mot_speed_set(self, 0.27f);
            break;
        }
        break;
    case 8:
        if ((system_w.field_0x0c & 0x1F) == 0x10) {
            fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
            if (fx != NULL) {
                copyVec3(&offset, setVec3(&sp2C, fx->x, fx->y, fx->z));
                eft013_em_place(self, &offset, fx->joint, fx->scale, 0xA);
            } else {
                copyVec3(&offset, setVec3(&sp20, 0.0f, 100.0f, 0.0f));
                eft013_em_place(self, &offset, 2, 2.0f, 0xA);
            }
        }
        if (em_demo_time_ck(0x47C) == 1) {
            self->state++;
            em_mot_set(self, 0x6B, 0, 0);
            em_demo_pos_set(self, 57.4f, 4.0f, -169.9f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
            break;
        }
        break;
    case 9:
        if ((system_w.field_0x0c & 0x1F) == 0x10) {
            fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
            if (fx != NULL) {
                copyVec3(&offset, setVec3(&sp14, fx->x, fx->y, fx->z));
                eft013_em_place(self, &offset, fx->joint, fx->scale, 0xA);
                break;
            }
            copyVec3(&offset, setVec3(&sp8, 0.0f, 100.0f, 0.0f));
            eft013_em_place(self, &offset, 2, 2.0f, 0xA);
        }
        break;
    }
}

/* r3 the work record; the death/thrash step (motion 0x6B) with the effect-call pair. */
void em027_act13_6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x6B, 0, 0);
        em_demo_pos_set(self, 57.4f, 4.0f, -169.9f);
        em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em027_act13_7(struct _ENEMY_WORK* self) {

    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 0, 0);
        em_demo_pos_set(self, 930.4f, 7.0f, -8910.3f);
        em_demo_rot_set(self, 0.0f, 11.25f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x10E) == 1) {
            self->state++;
            em_mot_set(self, 3, 0, 0);
            em_demo_pos_set(self, 930.4f, 7.0f, -8910.3f);
            em_demo_rot_set(self, 0.0f, 11.25f, 0.0f);
            break;
        }
    default:
        break;
    case 2:
        if (em_demo_time_ck(0x3A2) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 1030.8f, 7.0f, -5634.0f);
            em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
            break;
        }
        break;
    case 3:
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0x14);
            break;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x42A) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 1030.8f, 7.0f, -5968.7f);
            em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
            break;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x49C) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
            em_demo_pos_set(self, 1030.8f, 7.0f, -6565.9f);
            em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
            break;
        }
        break;
    case 6:
        if (em_frame_check(self, 1, 24.0f, 0.0f) == 0) {
            self->pos.x = 1030.8f + (-738.9f * (em_mot_frame_get(self) / 24.0f));
            self->pos.z = -6565.9f + (48.799805f * (em_mot_frame_get(self) / 24.0f));
        } else {
            em_demo_pos_set(self, 291.9f, 7.0f, -6517.1f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x68, 4, 0);
            em_demo_pos_set(self, 291.9f, 7.0f, -6517.1f);
            em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
            break;
        }
        break;
    case 7:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 1) {
            em_flag_0x115_set(self);
        }
        break;
    }
}

/* r3 the work record; the death step (motion 0x68) with the effect-call pair. */
void em027_act13_8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x68, 0, 0x32);
        em_demo_pos_set(self, 291.9f, 7.0f, -6517.1f);
        em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em027_act13_9(struct _ENEMY_WORK* self) {

    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 0, 0x3A);
        em_demo_pos_set(self, 959.4f, 7.0f, -9565.2f);
        em_demo_rot_set(self, 0.0f, 16.5f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x10E) == 1) {
            self->state++;
            em_mot_set(self, 3, 0, 0x3A);
            em_demo_pos_set(self, 959.4f, 7.0f, -9565.2f);
            em_demo_rot_set(self, 0.0f, 16.5f, 0.0f);
            break;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x3A2) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 1873.8f, 7.0f, -5876.8f);
            em_demo_rot_set(self, 0.0f, 187.5f, 0.0f);
            break;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x572) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0x14);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 1215.9f, 7.0f, -7211.3f);
            em_demo_rot_set(self, 0.0f, 189.7f, 0.0f);
        }
        break;
    }
}

/* r3 the work record; the death step (motion 4) with the effect-call pair. */
void em027_act13_10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 0, 0);
        em_demo_pos_set(self, 1035.8f, 0.0f, -4736.0f);
        em_demo_rot_set(self, 0.0f, -189.7f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em027_act13_11(struct _ENEMY_WORK* self) {

    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 0, 0x6A);
        em_demo_pos_set(self, 571.9f, 7.0f, -9051.8f);
        em_demo_rot_set(self, 0.0f, 12.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(0x10E) == 1) {
            self->state++;
            em_mot_set(self, 3, 0, 0x6A);
            em_demo_pos_set(self, 571.9f, 7.0f, -9051.8f);
            em_demo_rot_set(self, 0.0f, 12.0f, 0.0f);
            break;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x3A2) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0x14);
            em_demo_pos_set(self, 1469.7f, 7.0f, -6226.3f);
            em_demo_rot_set(self, 0.0f, 189.25f, 0.0f);
            break;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x554) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0);
            em_mot_speed_set(self, 1.5f);
            em_demo_pos_set(self, 832.8f, 7.0f, -7309.8f);
            em_demo_rot_set(self, 0.0f, 191.5f, 0.0f);
            break;
        }
        break;
    case 4:
        if (em_frame_check(self, 1, 94.0f, 0.0f) == 0) {
            self->state++;
            em_mot_set(self, 4, 0, 0x14);
            em_mot_speed_set(self, 1.5f);
        }
        break;
    }
}

void em027_act13_12(struct _ENEMY_WORK* self) {

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 0, 0x14);
        em_mot_speed_set(self, 1.5f);
        em_demo_pos_set(self, 1556.6f, 5.2f, -4202.3f);
        em_demo_rot_set(self, 0.0f, -191.5f, 0.0f);
        em_demo_enable(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

void em027_act13_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em027_act13_0(self);
        break;
    case 1:
        em027_act13_1(self);
        break;
    case 2:
        em027_act13_2(self);
        break;
    case 3:
        em027_act13_3(self);
        break;
    case 4:
        em027_act13_4(self);
        break;
    case 5:
        em027_act13_5(self);
        break;
    case 6:
        em027_act13_6(self);
        break;
    case 7:
        em027_act13_7(self);
        break;
    case 8:
        em027_act13_8(self);
        break;
    case 9:
        em027_act13_9(self);
        break;
    case 10:
        em027_act13_10(self);
        break;
    case 11:
        em027_act13_11(self);
        break;
    case 12:
        em027_act13_12(self);
        break;
    }
}

s32 em027_alert_ck(struct _ENEMY_WORK* self) {
    _ENEMY_DATA* data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);

    if (data != NULL) {
        if (self->area_no == data->field_0x14 && self->field_0x00A != 0 && self->field_0x43B == 0) {
            return 1;
        }
    } else if (self->field_0x00A != 0 && self->field_0x43B == 0) {
        return 1;
    }
    return 0;
}

void em027_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em027_act0_dispatch(self);
        break;
    case 1:
        em027_act1_dispatch(self);
        break;
    case 2:
        em027_act2_dispatch(self);
        break;
    case 7:
        em027_act7_dispatch(self);
        break;
    case 10:
        em027_act10_se_play(self);
        break;
    case 11:
        em027_act11_se_play(self);
        break;
    case 12:
        em027_act12_dispatch(self);
        break;
    case 13:
        em027_act13_dispatch(self);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

void em027_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, s32 joint, s32 delta, f32 scale) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (kind) {
    case 0:
        eft009_spawn_at_joint(self, joint, type, delta, scale);
        break;
    case 1:
        get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&pos, self->area_no, type, delta, scale);
        break;
    }
}

void em027_motion_events(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 shift;

    VEC3_ctor(&pos);
    VEC3_ctor(&shift);
    if (self->team == 29) {
        return;
    }
    switch (em_get_mot_no(self)) {
    case 2:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 11, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 7, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 11, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 7, 0, 1.0f);
            }
        }
        break;
    case 4:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 20, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 23, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 23, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 20, 0, 1.0f);
            }
        }
        break;
    case 7:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 538.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 20, 0, 0.6f, &shift);
            }
        } else {
            if (em_after_frame_check(self, 0, 538.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 36, 0, 1.2f, &shift);
            }
        }
        break;
    case 8:
        if (self->action != 13 || self->state_sub != 3) {
            if (self->field_0x228 & 6) {
                if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
                    setVector3(&shift, -60.0f, 0.0f, 0.0f);
                    eft009_spawn_at_pos(self, 7, 20, 0, 0.6f, &shift);
                }
            } else {
                if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
                    get_joint_wpos_em(self, 7, &pos);
                    pos.x -= 60.0f;
                    pos.y = self->field_0x20C;
                    eft_spawn_pos_in_area(&pos, self->area_no, 7, self->field_0x1C0, 1.2f);
                }
            }
        }
        break;
    case 9:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 44.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 546.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 636.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 20, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 120.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 356.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 608.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 666.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 23, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 540.0f, 0.0f) == 1) {
                setVector3(&shift, -60.0f, 0.0f, 0.0f);
                eft009_spawn_at_pos(self, 7, 20, 0, 0.6f, &shift);
            }
        } else {
            if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 44.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 546.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 636.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 20, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 120.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 356.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 608.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 666.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 23, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 540.0f, 0.0f) == 1) {
                get_joint_wpos_em(self, 7, &pos);
                pos.x -= 60.0f;
                pos.y = self->field_0x20C;
                eft_spawn_pos_in_area(&pos, self->area_no, 7, self->field_0x1C0, 1.2f);
            }
        }
        break;
    case 19:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 25, 23, 0, 1.8f);
                em027_effect_spawn(self, 0, 25, 20, 0, 1.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 23, 0, 1.8f);
                em027_effect_spawn(self, 0, 9, 20, 0, 1.8f);
            }
        }
        break;
    case 27:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 96.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 20, 0, 3.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 23, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 96.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 23, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 20, 0, 1.0f);
            }
        }
        break;
    case 28:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 7, 0, 3.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 9, 7, 0, 1.2f);
            }
        }
        break;
    case 102:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 20, 3, 0, 0.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.5f);
            }
        }
        break;
    case 103:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 43.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 88.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 23, 3, 0, 8.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 43.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 88.0f, 0.0f) == 1
                || em_after_frame_check(self, 0, 188.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.0f);
            }
        }
        break;
    case 106:
        if (self->field_0x228 & 6) {
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 0, 20, 3, 0, 0.8f);
            }
        } else {
            if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
                em027_effect_spawn(self, 1, 7, 3, self->field_0x1C0, 2.5f);
            }
        }
        break;
    }
}

u8 em027_state_ck(struct _ENEMY_WORK* self, u8 kind) {
    if (kind == 0) {
        return self->em027_0x328.seated_0x328 == 1;
    }
    return 0;
}

s32 em027_event_request(void) {
    lb_event_request(4);
    return 0;
}

void em027_area_start_set(struct _ENEMY_WORK* self, s8* action, s8* sub) {
    _ENEMY_DATA* data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);

    if (data != NULL) {
        self->area_no = data->field_0x14;
        self->field_0x9F6 = data->field_0x15;
    }
    em027_area_target_set(self);
    em_move_mode_set(self, 0);
    *action = 12;
    if (data != NULL) {
        switch (data->field_0x08) {
        case 0:
            *sub = 0;
            break;
        case 2:
            *sub = 1;
            break;
        case 3:
            *sub = 2;
            break;
        case 4:
            *sub = 1;
            break;
        case 5:
            if (data->field_0x0B == 0) {
                *sub = 0;
            } else {
                *sub = 1;
            }
            break;
        default:
            *sub = 0;
            break;
        }
    } else {
        *sub = 1;
    }
}

void em027_seated_clear(struct _ENEMY_WORK* self) {
    self->em027_0x328.seated_0x328 = 0;
    self->seated_keep_0xB05 = 0;
}

void em027_mini_init(_ENEMY_MINI_WORK* mini, u8 mode) {
    _ENEMY_DATA* data;
    s8 variant;

    if (mini->resume_mode_0x34 == 1) {
        mini->field_0x07 = mini->resume_mode_0x34;
    }
    switch (mode) {
    case 0:
        if (mini->field_0x10 == 0) {
            variant = em027_ground_variant_roll(mini->order_0x16);
            if (variant == 1) {
                mini->team_0x03 = 0;
            } else if (variant == 2) {
                mini->team_0x03 = 3;
            }
        }
        break;
    case 1:
    case 4:
        variant = em027_ground_variant_roll(mini->order_0x16);
        if (variant == 1) {
            mini->team_0x03 = 0;
        } else if (variant == 2) {
            mini->team_0x03 = 3;
        }
        data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(mini->monster_0x02, mini->team_0x03), mini->entry_0x09);
        if (data != NULL) {
            mini->area_0x0A = data->field_0x14;
            mini->area_prev_0x0B = data->field_0x15;
            switch (data->field_0x08) {
            case 0:
                mini->resume_mode_0x34 = 1;
                break;
            case 5:
                if (data->field_0x0B == 0) {
                    mini->resume_mode_0x34 = 1;
                } else {
                    mini->resume_mode_0x34 = 2;
                }
                break;
            default:
                mini->resume_mode_0x34 = 2;
                break;
            }
        }
        break;
    }
}

void em027_mini_step(_ENEMY_MINI_WORK* mini) {
    _ENEMY_DATA* data;
    u8 state;

    mini->resume_mode_0x34 = 0;
    data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(mini->monster_0x02, mini->team_0x03), mini->entry_0x09);
    if (data == NULL) {
        return;
    }
    state = mini->roam_state_0x20;
    switch (state) {
    case 0:
        if (mini->area_0x0A != data->field_0x14) {
            mini->roam_state_0x20 = 2;
            mini->move_timer_0x22 = ran_suu_c1(1) & 0x3F;
            break;
        }
        switch (data->field_0x08) {
        case 5:
            if (data->field_0x0B == 0) {
                mini->roam_state_0x20 = 1;
            } else {
                mini->roam_state_0x20 = 4;
            }
            mini->move_timer_0x22 = (u8)ran_suu_c1(1) + 0x5A;
            if (data->field_0x0B == 0) {
                mini->seated_keep_0x35 = 1;
            }
            break;
        case 4:
            if (mini->field_0x07 == 1) {
                mini->roam_state_0x20 = 3;
            } else {
                mini->roam_state_0x20 = 2;
            }
            mini->move_timer_0x22 = (u8)ran_suu_c1(1) + 0x5A;
            break;
        case 3:
            mini->field_0x07 = 1;
            break;
        }
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        mini->resume_mode_0x34 = state;
        if (mini->move_timer_0x22 <= 0) {
            if (eft_slot_armed_ck((struct EftSlot*)data) == 1 || mini->area_0x0A != data->field_0x14) {
                mini->roam_state_0x20 = 0;
                mini->move_timer_0x22 = 0;
                mini->area_prev_0x0B = mini->area_0x0A;
                if (data->field_0x0F == 3) {
                    mini->area_0x0A = em_kind_area_next(mini->monster_0x02, mini->team_0x03, mini->area_0x0A,
                                                        data->field_0x10, ran_suu_c1(1));
                } else {
                    mini->area_0x0A = em_kind_area_next(mini->monster_0x02, mini->team_0x03, mini->area_0x0A,
                                                        data->field_0x14, ran_suu_c1(1));
                }
                mini->seated_keep_0x35 = 0;
            } else {
                mini->move_timer_0x22 = ran_suu_c1(1) & 0x3F;
            }
        } else {
            mini->move_timer_0x22--;
            if (mini->area_0x0A != data->field_0x14 && mini->move_timer_0x22 > 0x5A) {
                mini->move_timer_0x22 = ran_suu_c1(1) & 0x3F;
            }
        }
        break;
    }
}

}
