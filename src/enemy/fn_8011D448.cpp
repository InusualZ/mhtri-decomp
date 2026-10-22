/* enemy/fn_8011D448.cpp - the enemy effect family (the pooled `_EFT` creators an `_ENEMY_WORK` owns, with their
 *   release/dispatch stages), the enemy status bits, health gauge and part tables, the per-frame damage settle with
 *   its 27 reaction handlers, and the action-script runner `em_se_tbl_play` drives.
 * RANGE. .text 0x8011D448-0x801251D0 (101 functions); .data 0x805A0C10-0x805A0FB0 (the effect tables, the kill-report
 *   table and four jump tables), .sdata 0x807919C8-0x807919D0, .sdata2 0x80796C08-0x80796C50, extab, extabindex.
 * SEAM. The `.sdata2` run holds 0.0f twice (0x80796C08, read only by the effect family 0x8011D448-0x8011E5EC, and
 *   0x80796C40, read by the action script): one value pooled twice is two TUs.  Emitted from this source's literals,
 *   0x80796C10-0x80796C4C comes out byte-identical, so the second TU starts at `em_status_bits_init` (0x8011E5EC);
 *   the seam request carries the lines.  `ef/eft029_fx.cpp` below, `enemy/em_common.cpp` above.
 * NAMES. `em_parts_damage_level_get` (0x8011E9DC) is a runtime-dump name; the dump's `JASSeqCtrl::setIntrMask` at
 *   0x8011F230 is a library-signature match on a 16-byte body, not evidence.  Every other name is a GUESS from its
 *   body and callers: `em_eft_fx_*` (the `eft029_*` create/release/dispatch/start/move/state_next/free scheme; the
 *   `em_eft_fx_family_tbl` tag picks the grouped or flat body), `em_status_bits_*` (+0x824), `em_gauge_*` (+0x7A0),
 *   `em_parts_*` (+0x838), `em_damage_*` (the frame's damage record +0x7CA..+0x80E; `em_damage_eval` is
 *   `fn_8013823C`'s per-frame call), `em_status_buildup_*` (+0x802[n-9]), `em_hit_dir_*` (+0x1C0 against +0x816),
 *   `em_event_settle_kindN` (the handler of reaction kind N at +0x82C; kinds 1/27 keep `em_event_settle`, `_kindN_ck`
 *   the hook slots 0xF/0x10/0x12/0x13 those kinds test), `em_kill_sub_kind_ck` (enemy/em_common.cpp's lane proposal),
 *   `em_seq_*` (the 8-byte {code, argument} action script), and the data names `em_eft_fx_*_tbl`/`_ids`.
 * RESIDUALS. All 101 rows written; partial rows:
 *  - `em_eft_fx_dispatch`: retail lays the stage-0 body after the three tail calls behind its own `b` (95.56).
 *  - `em_eft_fx_start_groups`, `em_damage_eval`, `em_parts_refresh`: register colouring only (the
 *    declaration orders `permdecl`/`hill` found do not reach it).
 *  - `em_seq_move_setup`, `em_seq_move_update`: retail lowers the move-kind switch into `subi`+`cmplwi` range tests
 *    (1-3, 7-8, then 5/4/6); every switch and if spelling tried gives a `cmpwi` tree or the wrong layout.
 *  - `em_seq_exec`: the jitter-mode test keeps the 0-2/4-6/3/7 ranges apart (ours merges them).
 *  - `em_seq_run`: the snore index's `(tick & 0x7F) + group * 31` schedules the tick load later.
 *  - data: `.sdata2` lacks the effect family's 0.0f (0x80796C08, declared `extern`; the seam above), `.data` the
 *    4-byte pad at 0x805A0FAC.
 * SHAPES. `#pragma peephole off` (retail keeps the unfused `clrlwi`+`slwi`/`stb` pairs), `fp_contract off` (no fused
 *   multiply-adds in the target) and `pool_data off` (the tables are defined here); the creators take `u32`
 *   type/timer arguments; the reactions' inner `switch (em_hit_dir_get(...))` list `default:` first; the move/flag
 *   checks of `enemy/em_common.cpp` return `u32` (retail compares them `cmplwi`); `em_get_part_limit`,
 *   `em_roster_slot_effect_set`, `em_ground_ck`, `UpdateValue` and `se_em_snore_req` are called through casts where
 *   the owner's declared parameters differ from the registers retail passes.
 */

#include "types.h"

#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"

#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft_res.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h" /* get_em_scale / em_get_mot_no (rule 2) */
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck (rule 2) */
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/em_hook_get.h"
#include "enemy/em_act_trap_ck.h"
#include "enemy/em_hook9_call.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/em_motion_timer_arm.h"
#include "enemy/em_mot_end_ck.h"
#include "enemy/em_pop.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_rot_vec_copy.h"
#include "enemy/em_flag_0x8AE_ck.h"
#include "enemy/em_model.h"
#include "enemy/em025_prog.h"
#include "hud/em_net_send.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/fn_8030121C.h"
#include "Pl/pl_skill.h"
#include "ef/get_move_work_adrs.h"
#include "stage/get_now_mapno.h"
#include "stage/stage_zone_hit_ck.h"
#include "sound/se.h"
#include "sound/fn_800D7F54.h"
#include "enemy/fn_80138074.h" /* em_user_data_reenter (rule 2) */

/* The unit's codegen pragmas (the unit header's SHAPES line). */
#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* This unit's own `.data` (0x805A0C10-0x805A0FAC: the tables below, then the compiler's four jump tables) and
 * `.sdata` (0x807919C8-0x807919D0). */
extern "C" {
/* [type * 2 + group] effect id of the grouped family for the default owner kind. */
u16 em_eft_fx_group_ids[8] = { 0x119, 0x11A, 0x11B, 0x11C, 0x11D, 0x11E, 0x143, 0x143 };
/* [type - 4] effect id of the flat family. */
u16 em_eft_fx_flat_ids[8] = { 0x1AB, 0x144, 0x1C1, 0x1FD, 0x26A, 0x269, 0x1FD, 0x1FD };
/* The grouped ids for owner kinds 15, 16 and 18. */
u16 em_eft_fx_group_ids_em15[8] = { 0x4D7, 0x4D8, 0x4D9, 0x4DA, 0x4DB, 0x4DC, 0x4DD, 0x4DD };
u16 em_eft_fx_group_ids_em16[8] = { 0xA2B, 0xA2C, 0x48D, 0x48E, 0xA2D, 0xA2E, 0x48F, 0x490 };
u16 em_eft_fx_group_ids_em18[8] = { 0xA31, 0xA32, 0x11B, 0x11C, 0xA33, 0xA34, 0x143, 0x143 };
/* [type] effect count the flat-family creators store. */
u8 em_eft_fx_flat_count_tbl[16] = { 15, 15, 15, 15, 1, 1, 1, 2, 1, 1, 2, 1 };
/* [type] resource group of the flat family. */
u16 em_eft_fx_flat_group_tbl[12] = { 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x26, 0x24, 0x24, 0x26, 0x26 };
/* [type] offset z of the target vector. */
f32 em_eft_fx_offset_z_tbl[12] = { 0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, 800.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
/* [type] family tag (0 = grouped, 1 = flat). */
u8 em_eft_fx_family_tbl[16] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 };
/* [sub] the kill-report kind `em_kill_sub_kind_ck` matches. */
u8 em_kill_sub_kind_tbl[0x3C] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x01, 0x01, 0x01, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B,
    0x1B, 0x1B, 0x1B, 0x14, 0x01, 0x01, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00
};
/* [type * 2 + group] re-arm delay of a grouped slot. */
s8 em_eft_fx_group_delay_tbl[8] = { 4, 4, 2, 2, 7, 7, 5, 0x50 };
extern const f32 em_eft_fx_zero; /* 0.0f */
}

/* This unit's own members, used before their definitions. */
extern "C" {
void em_eft_fx_start_groups(_EFT* self);
void em_eft_fx_start_flat(_EFT* self);
void em_eft_fx_move(_EFT* self);
void em_eft_fx_move_groups(_EFT* self);
void em_eft_fx_move_flat(_EFT* self);
void em_eft_fx_fade(_EFT* self);
void em_eft_fx_fade_groups(_EFT* self);
void em_eft_fx_state_next(_EFT* self);
void em_eft_fx_free(_EFT* self);
void em_eft_fx_place_at_joint(_EFT* self, nw4r::ef::Effect* effect, u8 group);
void em_gauge_add(_ENEMY_WORK* self, s32 step, s32 limit);
void em_eft_fx_release_groups(_EFT* self);
void em_eft_fx_release_flat(_EFT* self);
void em_eft_fx_release(_EFT* self);
void em_eft_fx_dispatch(_EFT* self);
_EFT* em_eft_fx_create(_ENEMY_WORK* self, u32 kind, u32 id0, u32 id1, f32 scale, u8 timer);
void em_gauge_step(_ENEMY_WORK* self, s32 step);
void em_gauge_set(_ENEMY_WORK* self, s32 value);
void em_damage_flags_set(_ENEMY_WORK* self, u16 mask);
u32 em_status_bits_ck(_ENEMY_WORK* self, u32 mask);
void em_parts_state_add(_ENEMY_WORK* self, u8 part, u8 add);
void em_parts_limit_reset(_ENEMY_WORK* self, u8 part);
u32 em_event_settle_kind14_ck(_ENEMY_WORK* self);
u32 em_event_settle_kind16_ck(_ENEMY_WORK* self);
u32 em_event_settle_kind11_ck(_ENEMY_WORK* self);
u32 em_event_settle_kind19_ck(_ENEMY_WORK* self);
u8 em_status_buildup_eval(_ENEMY_WORK* self);
s16 em_status_buildup_get(_ENEMY_WORK* self, u8 kind);
s16 em_damage_total_filter(_ENEMY_WORK* self, s32 total);
u8 em_hit_dir_mode_get(_ENEMY_WORK* self, u8 kind);
u8 em_hit_dir_turn_mode_get(_ENEMY_WORK* self, u8 kind);
s32 em_event_settle_dispatch(_ENEMY_WORK* self);
}

/* The effect record's per-family work blocks. */

/* One group of the grouped family: the live slots and the countdown to the next re-arm.  size: 0x44 */
struct EmEftFxGroup {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[15];
    /* +0x40 */ s8 delay;
    /* +0x41 */ u8 pad_0x41[3];
};

/* The grouped family's work block (the `0xA8` pool `em_eft_fx_create` asks for): two groups placed at the two
 * joints the creator passes, the scale, the offset the joints are pushed along, and the owner's motion the
 * effect lives for.  size: 0xA8 */
struct EmEftFxGroupWork {
    /* +0x00 */ s32 group_count;
    /* +0x04 */ EmEftFxGroup groups[2];
    /* +0x8C */ f32 scale;
    /* +0x90 */ u32 joints[2];
    /* +0x98 */ nw4r::math::VEC3 offset;
    /* +0xA4 */ u16 mot_no;
    /* +0xA6 */ u8 pad_0xA6[2];
};

/* The flat family's work block (the `0x20` pool): up to two effects at one joint.  size: 0x20 */
struct EmEftFxFlatWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[2];
    /* +0x0C */ f32 scale;
    /* +0x10 */ u32 joint;
    /* +0x14 */ nw4r::math::VEC3 pos;
};

/* The data records the part, damage and action-script code reads. */

/* One record of the enemy data's part-break list (`EnemyData::part_breaks_0x88`, closed by part 0xFF): the effect
 * and sound a part plays when it reaches `level`, and the group it breaks with.  size: 0x20 */
struct EmPartBreakRec {
    /* +0x00 */ u8 part;
    /* +0x01 */ u8 level;
    /* +0x02 */ u8 effect;            /* 0/1 pick the effect kind, others none */
    /* +0x03 */ u8 sound;             /* 0..3 pick the sound, others none */
    /* +0x04 */ u8 group;             /* bit 0x80: the group breaks when every other part is still below its level */
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u32 joint;            /* -1 = no effect */
    /* +0x0C */ nw4r::math::VEC3 offset;
    /* +0x18 */ f32 scale;
    /* +0x1C */ u32 color;
};

/* One part's flinch-limit record (`em_get_part_limit`): how many limit flinches the part has, the two hit kinds
 * that wear the limit down (0xFF = none) and the limit itself.  size: 0x6 (the fields read here) */
struct EmPartLimitRec {
    /* +0x00 */ u16 state_max_0x00;
    /* +0x02 */ u8 kind_a_0x02;
    /* +0x03 */ u8 kind_b_0x03;
    /* +0x04 */ s16 limit_0x04;
};

/* The extra data's part damage table (`EnemyData::extra`): one gauge per part.  size: 0x8 (the fields read here) */
struct EmPartHpTable {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ s16* hp;
};

/* The rank record `em_rank_rec_get` returns.  size: 0x1C (the table's stride) */
struct EmRankRec {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ f32 part_rate_0x0C;   /* scales every part's damage gauge */
    /* +0x10 */ u8 pad_0x10[0xC];
};

/* The attacker record `em_attacker_rec_get` returns.  size: 0xC (the fields read here) */
struct EmAttackerRec {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ u8 stun_ratio_0x08;   /* percent of a hit's value that builds stun */
    /* +0x09 */ u8 pad_0x09[3];
};

/* One player record of `get_move_work_adrs(2)` as the hate code reads it.  size: 0xB20 */
struct EmPlayerWork {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 pad_0x001[0xB1F];
};

/* One enemy record of `get_move_work_adrs(3)` as the hate code reads it (the records are 0xB18 apart).
 * size: 0xB18 */
struct EmEnemyRec {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 pad_0x001[2];
    /* +0x003 */ u8 team;
    /* +0x004 */ u8 pad_0x004[0xB14];
};

/* `em_get_part_limit` takes the part in r4; its header spells the one-argument form the net sync uses. */
static inline EmPartLimitRec* em_part_limit_get(_ENEMY_WORK* self, u8 part)
{
    return ((EmPartLimitRec* (*)(_ENEMY_WORK*, u8))em_get_part_limit)(self, part);
}

/* The move block a script motion record points at: the start velocity and acceleration, and the per-axis limits
 * (or, for the jittered move command, the jitter scales) with their modes.  size: 0x28 */
struct EmSeqMove {
    /* +0x00 */ nw4r::math::VEC3 velocity;
    /* +0x0C */ nw4r::math::VEC3 accel;
    /* +0x18 */ nw4r::math::VEC3 limit;
    /* +0x24 */ u8 mode[3];           /* [0] bit 0x80: unsigned jitter, bit 0x40: scale by the model */
    /* +0x27 */ u8 pad_0x27;
};

/* A script motion record (codes 0..2): the motion, its blend arguments, the wait mask and the move kind.
 * size: 0x18 */
struct EmSeqMotion {
    /* +0x00 */ u16 motion;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ s32 blend;
    /* +0x08 */ u32 frame;
    /* +0x0C */ u32 loop;
    /* +0x10 */ u16 flags;            /* the wait mask `em_seq_wait_ck` tests; 2 chains two records */
    /* +0x12 */ u8 move_kind;
    /* +0x13 */ u8 pad_0x13;
    /* +0x14 */ union {
        EmSeqMove* move;
        f32* range;
    };
};

/* A script condition (codes 9 and 17): the test and the table it jumps to.  size: 0xC */
struct EmSeqCond {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ s32 value;
    /* +0x08 */ struct EmSeqCmd* target;
};

/* The turn parameters of code 10.  size: 0xC */
struct EmSeqTurn {
    /* +0x00 */ f32 rate;
    /* +0x04 */ f32 window;
    /* +0x08 */ s32 angle;
};

/* The state code 0xFE hands the enemy to.  size: 0x2 */
struct EmSeqState {
    /* +0x00 */ u8 state;
    /* +0x01 */ u8 sub;
};

/* The roster effect code 15 binds.  size: 0x8 */
struct EmSeqRoster {
    /* +0x00 */ s32 base;
    /* +0x04 */ u8 kind;
    /* +0x05 */ u8 pad_0x05[3];
};

/* One 8-byte action-script record: the code and its argument.  size: 0x8 */
struct EmSeqCmd {
    /* +0x00 */ u8 code;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ union {
        s32 value;
        u32 count;
        EmSeqMotion* motion;
        EmSeqCmd* jump;
        EmSeqCond* cond;
        EmSeqTurn* turn;
        f32* floats;
        EmSeqRoster* roster;
        EmSeqMove* move;
        _CP_VECTOR* rot;
        EmSeqState* state;
        void (*hook)(_ENEMY_WORK*);
    };
};

/* The ailment-effect block (`EnemyData::ailment_fx_0x9C`) as the snore reads it: the sleep effect's record,
 * whose first word is the head joint.  size: 0x10 (the fields read here) */
struct EmAilmentFxView {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ u32* sleep_joint;
};

/* Bodies. */

/* 0x8011D448 - spawns one grouped effect and points its offset along the type table's z. */
extern "C" void em_eft_fx_spawn(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id, id, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        setVector3(&work->offset, em_eft_fx_zero, em_eft_fx_zero, em_eft_fx_offset_z_tbl[type]);
    }
}

/* 0x8011D4AC - the same spawn, with the offset copied from the caller's vector. */
extern "C" void em_eft_fx_spawn_at(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, const nw4r::math::VEC3* pos,
                                   f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id, id, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        copyVec3(&work->offset, pos);
    }
}

/* 0x8011D4FC - the same spawn with two joints. */
extern "C" void em_eft_fx_spawn_pair(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, u32 timer, f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id0, id1, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        setVector3(&work->offset, em_eft_fx_zero, em_eft_fx_zero, em_eft_fx_offset_z_tbl[type]);
    }
}

/* 0x8011D558 - the grouped family's creator: within the enemy's own area, fills the work block and stamps the
 * record (tag 0x1f, type, timer, owner, area, rotation), installs the release/dispatch pair, marks demo ownership. */
extern "C" _EFT* em_eft_fx_create(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, f32 scale, u8 timer)
{
    _EFT* effect;
    EmEftFxGroupWork* work;

    if (get_now_areano() != self->act_id) {
        return NULL;
    }

    effect = (_EFT*)eft_res_slot_get(0xA8);
    if (effect == NULL) {
        return NULL;
    }

    work = (EmEftFxGroupWork*)effect->work_0x38;
    work->group_count = 2;
    work->scale = scale * get_em_scale(self);
    work->joints[0] = id0;
    work->joints[1] = id1;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = timer;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* 0x8011D690 - the flat family's creator on the enemy's own joint. */
extern "C" void em_eft_fx_spawn_anchor(_ENEMY_WORK* self, u32 type, u32 joint, f32 scale)
{
    _EFT* effect;
    EmEftFxFlatWork* work;

    if (get_now_areano() != self->act_id) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (EmEftFxFlatWork*)effect->work_0x38;
    work->count = em_eft_fx_flat_count_tbl[type];
    work->scale = scale * get_em_scale(self);
    work->joint = joint;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    effect->timer_0x0C = 0;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
}

/* 0x8011D7B0 - the flat family's creator that belongs to nobody: the caller passes the type, position, rotation
 * and area, so `source_0x30` stays NULL. */
extern "C" void em_eft_fx_spawn_free(u32 type, const nw4r::math::VEC3* pos, const _CP_VECTOR* rot, f32 scale,
                                     u8 area, u8 demo)
{
    _EFT* effect;
    EmEftFxFlatWork* work;

    if (get_now_areano() != area) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (EmEftFxFlatWork*)effect->work_0x38;
    work->count = em_eft_fx_flat_count_tbl[type];
    work->scale = scale;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->source_0x30 = NULL;
    effect->area_0x44 = area;
    effect->rot_0x24 = *rot;
    copyVec3(&effect->pos_0x18, pos);
    effect->timer_0x0C = 0;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = demo;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
}

/* 0x8011D8C0 - the family's `.release` callback: hands the type's slots back. */
extern "C" void em_eft_fx_release(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_release_groups(self);
        break;
    case 1:
        em_eft_fx_release_flat(self);
        break;
    }
}

/* 0x8011D8F0 - the grouped release: every slot of every group, one call each, then the group count is cleared. */
extern "C" void em_eft_fx_release_groups(_EFT* self)
{
    EmEftFxGroupWork* work = (EmEftFxGroupWork*)self->work_0x38;
    s32 i;
    s32 j;

    for (i = 0; i < work->group_count; i++) {
        for (j = 0; j < work->groups[i].count; j++) {
            push_eft_effect_heap_num(&work->groups[i].slots[j], 1);
        }
    }
    work->group_count = 0;
}

/* 0x8011D97C - the flat release: the whole run in one call, then the count is cleared. */
extern "C" void em_eft_fx_release_flat(_EFT* self)
{
    EmEftFxFlatWork* work = (EmEftFxFlatWork*)self->work_0x38;

    push_eft_effect_heap_num(work->slots, work->count);
    work->count = 0;
}

/* The stage-0 body: creates the type's effects. */
static inline void em_eft_fx_start(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_start_groups(self);
        break;
    case 1:
        em_eft_fx_start_flat(self);
        break;
    }
}

/* 0x8011D9B8 - the family's `.dispatch` callback: `state_0x05` selects the stage. */
extern "C" void em_eft_fx_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        em_eft_fx_move(self);
        return;
    case 2:
        em_eft_fx_fade(self);
        return;
    case 3:
        em_eft_fx_free(self);
        return;
    default:
        return;
    }
    em_eft_fx_start(self);
}

/* 0x8011DA24 - the grouped start: one effect per group at its joint, the owner's motion latched, then the first
 * move. */
extern "C" void em_eft_fx_start_groups(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    s32 i;
    EmEftFxGroupWork* work;
    _ENEMY_WORK* owner;
    u16* ids;
    u16 group_id;

    MTX34_ctor(&mtx);
    work = (EmEftFxGroupWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    switch (self->demo_flag_0x08) {
    case 15:
        ids = em_eft_fx_group_ids_em15;
        group_id = 0x26;
        break;
    case 16:
        ids = em_eft_fx_group_ids_em16;
        group_id = 0x4C;
        break;
    case 18:
        ids = em_eft_fx_group_ids_em18;
        group_id = 0x45;
        break;
    default:
        ids = em_eft_fx_group_ids;
        group_id = 0x24;
        break;
    }
    for (i = 0; i < work->group_count; i++) {
        work->groups[i].slots[0] = res_eft_create(ids[i + self->type_0x02 * 2], group_id, 0);
        if (work->groups[i].slots[0] == NULL) {
            em_eft_fx_free(self);
            return;
        }
        em_eft_fx_place_at_joint(self, work->groups[i].slots[0], i);
        work->groups[i].count++;
        work->groups[i].delay = em_eft_fx_group_delay_tbl[i + self->type_0x02 * 2];
    }
    work->mot_no = em_get_mot_no(owner);
    self->flag_0x01 = 1;
    em_eft_fx_move(self);
}

/* 0x8011DB74 - the flat start: the type's effect (an owner-kind variant for types 5/6/8/9), a second one for
 * types 7/10, all placed at the joint pushed along the type's offset. */
extern "C" void em_eft_fx_start_flat(_EFT* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    EmEftFxFlatWork* work;
    _ENEMY_WORK* owner;
    u16 id;
    u16 group_id;
    s32 i;

    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);
    work = (EmEftFxFlatWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    id = em_eft_fx_flat_ids[self->type_0x02 - 4];
    group_id = em_eft_fx_flat_group_tbl[self->type_0x02];
    switch (self->type_0x02) {
    case 5:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4DE;
            group_id = 0x26;
            break;
        }
        break;
    case 6:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4EE;
            group_id = 0x26;
            break;
        case 20:
            id = 0x89E;
            group_id = 0x70;
            break;
        }
        break;
    case 8:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4D1;
            group_id = 0x26;
            break;
        case 16:
            id = 0xA30;
            group_id = 0x4C;
            break;
        case 20:
            id = 0x895;
            group_id = 0x70;
            break;
        }
        break;
    case 9:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4D0;
            group_id = 0x26;
            break;
        case 16:
            id = 0xA2F;
            group_id = 0x4C;
            break;
        case 20:
            id = 0x894;
            group_id = 0x70;
            break;
        }
        break;
    }
    work->slots[0] = res_eft_create(id, group_id, 0);
    if (work->slots[0] == NULL) {
        em_eft_fx_free(self);
        return;
    }
    switch (self->type_0x02) {
    case 7:
    case 10:
        work->slots[1] = res_eft_create(0x3B8, 0x26, 0);
        if (work->slots[1] == NULL) {
            em_eft_fx_free(self);
            return;
        }
        break;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    setVector3(&offset, em_eft_fx_zero, em_eft_fx_zero, em_eft_fx_offset_z_tbl[self->type_0x02]);
    mulVecMat(&offset, &mtx);
    if (owner != NULL) {
        if (em_work_die_ck(owner) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        get_joint_wpos_em(owner, work->joint, &self->pos_0x18);
        addVec3To(&self->pos_0x18, &offset);
        copyVec3(&work->pos, &self->pos_0x18);
    }
    mtx34_set_trans(&mtx, &self->pos_0x18);
    for (i = 0; i < work->count; i++) {
        work->slots[i]->SetRootMtx(mtx);
    }
    self->flag_0x01 = 1;
    em_eft_fx_move(self);
}

/* 0x8011DE2C - the stage-1 body. */
extern "C" void em_eft_fx_move(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_move_groups(self);
        break;
    case 1:
        em_eft_fx_move_flat(self);
        break;
    }
}

/* Copies the stage's ambient colour onto one pooled effect. */
static inline void em_eft_fx_stage_color(_GXColor* color, u8 area)
{
    u32 rgba = get_stg_eft_col(area, 1);

    color->r = (rgba & 0xFF000000) >> 24;
    color->g = (rgba & 0xFF0000) >> 16;
    color->b = (rgba & 0xFF00) >> 8;
    color->a = rgba;
}

/* 0x8011DE5C - the grouped move: while the owner lives and keeps its motion, every group re-arms one slot per
 * delay, every slot is scaled and stepped, and a live slot under water takes the stage colour. */
extern "C" void em_eft_fx_move_groups(_EFT* self)
{
    s32 i;
    s32 j;
    EmEftFxGroupWork* work;
    _ENEMY_WORK* owner;
    u16* ids;
    u16 group_id;
    _GXColor color;

    work = (EmEftFxGroupWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    if (em_work_die_ck(owner) != 0) {
        self->state_0x05++;
        return;
    }
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->timer_0x0C = 0;
        self->state_0x05++;
        return;
    }
    if (--self->timer_0x0C < 0 || work->mot_no != em_get_mot_no(owner)) {
        self->state_0x05++;
        self->timer_0x0C = 60;
        em_eft_fx_fade(self);
        return;
    }
    switch (self->demo_flag_0x08) {
    case 15:
        ids = em_eft_fx_group_ids_em15;
        group_id = 0x26;
        break;
    case 16:
        ids = em_eft_fx_group_ids_em16;
        group_id = 0x4C;
        break;
    case 18:
        ids = em_eft_fx_group_ids_em18;
        group_id = 0x45;
        break;
    default:
        ids = em_eft_fx_group_ids;
        group_id = 0x24;
        break;
    }
    for (i = 0; i < work->group_count; i++) {
        work->groups[i].delay--;
        if (work->groups[i].delay < 0 && work->groups[i].count < 15) {
            work->groups[i].slots[work->groups[i].count] = res_eft_create(ids[i + self->type_0x02 * 2], group_id, 0);
            if (work->groups[i].slots[work->groups[i].count] != NULL) {
                em_eft_fx_place_at_joint(self, work->groups[i].slots[work->groups[i].count], i);
                work->groups[i].count++;
                work->groups[i].delay = em_eft_fx_group_delay_tbl[i + self->type_0x02 * 2];
            }
        }
        for (j = 0; j < work->groups[i].count; j++) {
            change_paramscale_eff(work->groups[i].slots[j], work->scale);
            if (effect_move(work->groups[i].slots[j]) == 1) {
                if (get_camera_pos().y < em_eft_fx_zero) {
                    em_eft_fx_stage_color(&color, self->area_0x44);
                    change_color_eff(work->groups[i].slots[j], &self->pos_0x18, color);
                    eft_res_models_spawn(self, (void**)&work->groups[i].slots[j], 1, 1, NULL);
                }
            }
        }
    }
}

/* 0x8011E138 - the flat move: types 5 and 8-10 follow the owner's joint, every effect is scaled and stepped
 * (one that ends closes the stage), and under water the stage colour and the model pass are applied. */
extern "C" void em_eft_fx_move_flat(_EFT* self)
{
    EmEftFxFlatWork* work;
    _ENEMY_WORK* owner;
    s32 i;
    _GXColor color;

    work = (EmEftFxFlatWork*)self->work_0x38;
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if ((u32)(self->type_0x02 - 8) <= 2 || (s32)self->type_0x02 == 5) {
        owner = (_ENEMY_WORK*)self->source_0x30;
        if (em_work_die_ck(owner) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->timer_0x0C = 0;
            return;
        }
        get_joint_wpos_em(owner, work->joint, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->slots[i], &self->pos_0x18);
        }
    }
    for (i = 0; i < work->count; i++) {
        change_paramscale_eff(work->slots[i], work->scale);
        if (effect_move(work->slots[i]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            self->timer_0x0C = 0;
            return;
        }
    }
    if (get_camera_pos().y < em_eft_fx_zero) {
        em_eft_fx_stage_color(&color, self->area_0x44);
        for (i = 0; i < work->count; i++) {
            change_color_eff(work->slots[i], &self->pos_0x18, color);
        }
        if ((u32)(self->type_0x02 - 8) <= 1) {
            eft_res_models_spawn(self, (void**)&work->slots[0], 1, 1, &work->pos);
        } else {
            eft_res_models_spawn(self, (void**)&work->slots[0], 1, 1, NULL);
        }
    }
    switch (self->type_0x02) {
    case 7:
    case 10:
        eft_res_models_spawn(self, (void**)&work->slots[1], 1, 1, NULL);
        break;
    }
}

/* 0x8011E378 - the stage-2 body. */
extern "C" void em_eft_fx_fade(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_fade_groups(self);
        break;
    case 1:
        em_eft_fx_state_next(self);
        break;
    }
}

/* 0x8011E3A8 - the grouped fade: the live slots run out their timer (stage colour under water), then the stage
 * ends. */
extern "C" void em_eft_fx_fade_groups(_EFT* self)
{
    s32 i;
    s32 j;
    EmEftFxGroupWork* work;
    _GXColor color;

    work = (EmEftFxGroupWork*)self->work_0x38;
    switch (self->field_0x06) {
    case 0:
        self->field_0x06++;
    case 1:
        if (--self->timer_0x0C >= 0) {
            for (i = 0; i < work->group_count; i++) {
                for (j = 0; j < work->groups[i].count; j++) {
                    if (effect_move(work->groups[i].slots[j]) == 1) {
                        if (get_camera_pos().y < em_eft_fx_zero) {
                            em_eft_fx_stage_color(&color, self->area_0x44);
                            change_color_eff(work->groups[i].slots[j], &self->pos_0x18, color);
                            eft_res_models_spawn(self, (void**)&work->groups[i].slots[j], 1, 1, NULL);
                        }
                    }
                }
            }
        } else {
            self->state_0x05++;
        }
        break;
    }
}

/* 0x8011E51C - closes the current stage. */
extern "C" void em_eft_fx_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011E52C - the terminal stage: hands the whole pooled record back. */
extern "C" void em_eft_fx_free(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x8011E530 - places `effect` at the owner's joint for `group`, pushed along the work's offset turned by the
 * record's rotation. */
extern "C" void em_eft_fx_place_at_joint(_EFT* self, nw4r::ef::Effect* effect, u8 group)
{
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* owner;
    EmEftFxGroupWork* work;

    VEC3_ctor(&joint);
    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);

    owner = (_ENEMY_WORK*)self->source_0x30;
    work = (EmEftFxGroupWork*)self->work_0x38;

    cpSetRotMatrix(&self->rot_0x24, &mtx);
    copyVec3(&offset, &work->offset);
    mulVecMat(&offset, &mtx);
    get_joint_wpos_em(owner, work->joints[group], &joint);
    addVec3To(&joint, &offset);
    mtx34_set_trans(&mtx, &joint);
    effect->SetRootMtx(mtx);
}

/* 0x8011E5EC - seeds the enemy's status bits from its static data record. */
extern "C" void em_status_bits_init(_ENEMY_WORK* self)
{
    self->bits_0x824 = get_enemy_data(self)->bits_0x30;
}

/* 0x8011E620 - sets status bits. */
extern "C" void em_status_bits_set(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 |= mask;
}

/* 0x8011E630 - clears status bits. */
extern "C" void em_status_bits_clr(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 &= ~mask;
}

/* 0x8011E640 - are all of `mask`'s bits set?  (retail emits the `cntlzw` of
 * `(bits & mask) - mask`, which is this comparison and nothing else.) */
extern "C" u32 em_status_bits_ck(_ENEMY_WORK* self, u32 mask)
{
    return (self->bits_0x824 & mask) == mask;
}

/* 0x8011E658 - steps the gauge and clamps it to [0, ceiling], never below the data record's per-model ratio
 * (`field_0x7a4 * field_0x7B0`). */
extern "C" void em_gauge_add(_ENEMY_WORK* self, s32 step, s32 ceiling)
{
    s32 value;
    s32 low;

    if (self->amount_0x7A0 > ceiling) {
        ceiling = self->amount_0x7A0;
    }
    value = self->amount_0x7A0 += step;
    if (value < 0) {
        self->amount_0x7A0 = 0;
    } else if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
    }
    low = (s32)((f32)(s32)self->field_0x7a4 * self->field_0x7B0);
    if (self->amount_0x7A0 < low) {
        self->amount_0x7A0 = low;
    }
}

/* 0x8011E6E4 - steps the gauge by `step` with the data record's ratio as the ceiling. */
extern "C" void em_gauge_step(_ENEMY_WORK* self, s32 step)
{
    em_gauge_add(self, step, self->field_0x7a4);
}

/* 0x8011E6EC - steps the gauge on every `period`-th system tick (an unsigned `divwu` modulo: a `u32` counter
 * against the sign-extended `s16`). */
extern "C" void em_gauge_period_step(_ENEMY_WORK* self, s32 step, s16 period, f32 scale)
{
    if (system_w.field_0x0c % period == 0) {
        em_gauge_add(self, step, (s32)((f32)(s32)self->field_0x7a4 * scale));
    }
}

/* 0x8011E760 - forces the gauge to `value`, clamped the same way `em_gauge_add` clamps it. */
extern "C" void em_gauge_set(_ENEMY_WORK* self, s32 value)
{
    s32 ceiling;
    s32 low;

    if (value < 0) {
        self->amount_0x7A0 = 0;
        return;
    }
    ceiling = (s32)self->field_0x7a4;
    if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
        return;
    }
    low = (s32)((f32)ceiling * self->field_0x7B0);
    if (value < low) {
        self->amount_0x7A0 = low;
    } else {
        self->amount_0x7A0 = value;
    }
}

/* 0x8011E7D8 - accumulates one part's damage level. */
extern "C" void em_parts_damage_add(_ENEMY_WORK* self, u8 part, u8 add)
{
    self->parts_0x838[part].damage_level += add;
}

/* 0x8011E7F4 - refills one part's damage gauge from the data table, scaled by the rank record. */
extern "C" void em_parts_hp_reset(_ENEMY_WORK* self, u8 part)
{
    f32 rate = ((EmRankRec*)em_rank_rec_get(self))->part_rate_0x0C;

    self->parts_0x838[part].hp_0x02 = rate * ((EmPartHpTable*)get_enemy_data(self)->extra)->hp[part];
}

/* 0x8011E898 - steps one part's state byte. */
extern "C" void em_parts_state_add(_ENEMY_WORK* self, u8 part, u8 add)
{
    self->parts_0x838[part].state_0x01 += add;
}

/* 0x8011E8B4 - refills one part's flinch-limit gauge, half again as large above level 40. */
extern "C" void em_parts_limit_reset(_ENEMY_WORK* self, u8 part)
{
    f32 rate;

    if (self->field_0x7c8 > 40) {
        rate = 1.5f;
    } else {
        rate = 1.0f;
    }
    self->parts_0x838[part].limit_0x04 = rate * em_part_limit_get(self, part)->limit_0x04;
}

/* 0x8011E960 - resets all eight parts. */
extern "C" void em_parts_init(_ENEMY_WORK* self)
{
    u8 i;

    for (i = 0; i < 8; i++) {
        self->parts_0x838[i].damage_level = 0;
        em_parts_hp_reset(self, i);
        self->parts_0x838[i].state_0x01 = 0;
        em_parts_limit_reset(self, i);
    }
}

/* 0x8011E9DC - one part's damage level; defined at C++ scope, where `(_ENEMY_WORK*, u8)` mangles to the map's
 * name. */
u8 em_parts_damage_level_get(_ENEMY_WORK* self, u8 part)
{
    return self->parts_0x838[part].damage_level;
}

/* 0x8011E9F0 - one part's state byte. */
extern "C" u8 em_parts_state_get(_ENEMY_WORK* self, u8 part)
{
    return self->parts_0x838[part].state_0x01;
}

/* 0x8011EA04 - marks a part group broken once: the quest's arena need, and the net message for a synced enemy. */
extern "C" void em_parts_break(_ENEMY_WORK* self, u8 group)
{
    s32 bit = 1 << group;

    if (self->broken_groups_0x014 & bit) {
        return;
    }
    ((void (*)(u8, u16))quest_arena_need_add)(self->team, self->field_0x01A);
    self->broken_groups_0x014 |= (s8)bit;
    if (self->field_0x1C8 & 8) {
        em_net_send(self, 5, 2);
    }
}

/* 0x8011EA90 - plays the break record of a part's new level (effect and sound at its joint), and breaks the part's
 * group once every record of the group has reached (or, for a 0x80 group, not reached) its level. */
extern "C" void em_parts_refresh(_ENEMY_WORK* self, u8 part, s32 flag)
{
    EmPartBreakRec* list;
    EmPartBreakRec* found;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 tmp;
    u32 inverted;
    EmPartBreakRec* rec;
    u32 group;
    u32 all;
    u32 (*hook)(_ENEMY_WORK*, u8);

    list = get_enemy_data(self)->part_breaks_0x88;
    found = NULL;
    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    inverted = 0;
    rec = list;
    if (list != NULL) {
        for (; rec->part != 0xFF; rec++) {
            if (rec->part == part && rec->level == em_parts_damage_level_get(self, part)) {
                if (em_area_ck(self) == 1 && rec->joint != (u32)-1) {
                    copyVec3(&pos, setVec3(&tmp, rec->offset.x, rec->offset.y, rec->offset.z));
                    switch (rec->effect) {
                    case 0:
                        eft_em_spawn_tinted(self, 0, rec->joint, &pos, rec->scale, rec->color);
                        break;
                    case 1:
                        eft_em_spawn_tinted(self, 2, rec->joint, &pos, rec->scale, rec->color);
                        break;
                    }
                    if (rec->sound <= 3) {
                        get_joint_wmat_em(self, rec->joint, &mtx);
                        mulVecMatAddTrans(&pos, &mtx);
                        switch (rec->sound) {
                        case 0:
                            se_req_pos_ps(self->se_handle_0xB14, 0x24, 2, &pos);
                            break;
                        case 1:
                            se_req_pos_ps(self->se_handle_0xB14, 0x40, 2, &pos);
                            break;
                        case 2:
                            se_req_pos_ps(self->se_handle_0xB14, 0x41, 2, &pos);
                            break;
                        case 3:
                            se_req_pos_ps(self->se_handle_0xB14, 0x42, 2, &pos);
                            break;
                        }
                    }
                }
                group = rec->group;
                if (group & 0x80) {
                    inverted = 1;
                }
                found = rec;
                break;
            }
        }
        if (found != NULL) {
            all = 1;
            for (rec = list; rec->part != 0xFF; rec++) {
                if (rec != found && rec->group == (u8)group) {
                    if (rec->level > em_parts_damage_level_get(self, rec->part)) {
                        if (inverted == 1) {
                            all = 0;
                            break;
                        }
                    } else if (inverted == 0) {
                        all = 0;
                        break;
                    }
                }
            }
            if (all == 1) {
                em_parts_break(self, group & ~0x80);
            }
        }
    }
    hook = (u32 (*)(_ENEMY_WORK*, u8))em_hook_get(self, 0x14);
    if (hook != NULL) {
        hook(self, part);
    }
}

/* 0x8011ED38 - scales one hit's damage by the enemy's rate: x3/x2 on a sleeping enemy (once per `mask`), x1.1 in a
 * trap, x1.1/x1.05 under status 4; never below 1. */
extern "C" s16 em_damage_bonus_apply(_ENEMY_WORK* self, s16 damage, u8 heavy, u32 mask)
{
    f32 rate;

    if (damage > 0) {
        rate = self->damage_rate_0x7B8;
        if (em_sleep_ck(self, 1) == 1 && (self->bonus_mask_0x7FC & mask) == 0) {
            if (heavy == 0) {
                rate *= 3.0f;
            } else {
                rate *= 2.0f;
            }
            self->bonus_mask_0x7FC = mask;
        } else if (em_act_trap_ck(self) == 1) {
            rate *= 1.1f;
        } else if (em_status_ck(self, 4) == 1) {
            if (self->field_0x7c8 <= 20) {
                rate *= 1.1f;
            } else {
                rate *= 1.05f;
            }
        }
        damage = damage * rate;
        if (damage <= 0) {
            damage = 1;
        }
    }
    return damage;
}

/* 0x8011EE84 - clears the frame's damage record and latches the facing the hit directions are measured from. */
extern "C" void em_damage_record_clear(_ENEMY_WORK* self)
{
    self->hit_0x833 = 0;
    self->field_0x828 = 0;
    self->part_damage_0x7CA[0] = 0;
    self->kind_damage_0x7DA[0][0] = 0;
    self->kind_damage_0x7DA[1][0] = 0;
    self->part_damage_0x7CA[1] = 0;
    self->kind_damage_0x7DA[0][1] = 0;
    self->kind_damage_0x7DA[1][1] = 0;
    self->part_damage_0x7CA[2] = 0;
    self->kind_damage_0x7DA[0][2] = 0;
    self->kind_damage_0x7DA[1][2] = 0;
    self->part_damage_0x7CA[3] = 0;
    self->kind_damage_0x7DA[0][3] = 0;
    self->kind_damage_0x7DA[1][3] = 0;
    self->part_damage_0x7CA[4] = 0;
    self->kind_damage_0x7DA[0][4] = 0;
    self->kind_damage_0x7DA[1][4] = 0;
    self->part_damage_0x7CA[5] = 0;
    self->kind_damage_0x7DA[0][5] = 0;
    self->kind_damage_0x7DA[1][5] = 0;
    self->part_damage_0x7CA[6] = 0;
    self->kind_damage_0x7DA[0][6] = 0;
    self->kind_damage_0x7DA[1][6] = 0;
    self->part_damage_0x7CA[7] = 0;
    self->kind_damage_0x7DA[0][7] = 0;
    self->kind_damage_0x7DA[1][7] = 0;
    self->ailment_0x802[0] = 0;
    self->ailment_0x802[1] = 0;
    self->ailment_0x802[2] = 0;
    self->ailment_0x802[3] = 0;
    self->part_hit_0x800 = 0;
    self->stun_0x80A = 0;
    self->exhaust_0x80C = 0;
    self->mask_0x80E = 0;
    self->bonus_mask_0x7FC = 0;
    self->hit_angle_0x816 = self->pos_0x1BC.y;
}

/* 0x8011EF24 - adds one hit's damage to a part, and to the part's slot of the hit kind. */
extern "C" void em_damage_hp_add(_ENEMY_WORK* self, u8 part, s16 damage, u8 heavy)
{
    s16 amount = em_damage_bonus_apply(self, damage, heavy, 1);
    s8 slot;

    self->part_damage_0x7CA[part] += amount;
    slot = em_part_slot_get(self, 0x7F);
    if (slot != -1) {
        self->kind_damage_0x7DA[slot][part] += amount;
    }
}

/* 0x8011EFBC - adds one hit's attribute: kinds 0..8 to the part's slot of that kind, 9..12 to the ailment build-up
 * (halved when an effect is active... scaled by 1.5), 13/14 to the net flags by strength. */
extern "C" void em_damage_attr_add(_ENEMY_WORK* self, u8 part, u8 kind, s16 base, s16 value, u8 heavy)
{
    s8 slot;

    switch (kind) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        slot = ((s8 (*)(_ENEMY_WORK*, u8))em_part_slot_get)(self, kind);
        if (slot != -1) {
            self->kind_damage_0x7DA[slot][part] += em_damage_bonus_apply(self, base + value, heavy, slot == 0 ? 4 : 8);
        }
        break;
    case 9: case 10: case 11: case 12:
        if (em_eft_active_ck(self) == 1) {
            value = value * 1.5f;
        }
        self->ailment_0x802[kind - 9] += value;
        break;
        case 13:
            if (value >= 150) {
                em_net_flag_apply_6(self);
            } else {
                em_net_flag_apply_3(self);
            }
            break;
        case 14:
            em_damage_flags_set(self, 2);
            if (value >= 200) {
                em_net_flag_apply_7(self);
            } else if (value >= 180) {
                em_net_flag_apply_4(self);
            } else if (value >= 150) {
                em_net_flag_apply_5(self);
            } else {
                em_net_flag_apply_2(self);
            }
            break;
    }
}

/* 0x8011F14C - marks a part hit this frame. */
extern "C" void em_damage_part_hit_set(_ENEMY_WORK* self, u8 part)
{
    self->part_hit_0x800 |= (u8)(1 << part);
}

/* 0x8011F16C - adds one hit's stun (through the attacker's stun ratio) and exhaust build-up. */
extern "C" void em_damage_stun_add(_ENEMY_WORK* self, u8 unused, u8 attacker, u8 value)
{
    u8 ratio;
    s16 stun;

    if (value != 0) {
        ratio = ((EmAttackerRec*)em_attacker_rec_get(self, attacker))->stun_ratio_0x08;
        if (ratio != 0) {
            stun = (f32)value * (f32)ratio / 100.0f;
            if (stun <= 0) {
                stun = 1;
            }
            self->stun_0x80A += stun;
        }
        self->exhaust_0x80C += value;
    }
}

/* 0x8011F230 - ORs bits into the frame's damage flags. */
extern "C" void em_damage_flags_set(_ENEMY_WORK* self, u16 mask)
{
    self->mask_0x80E |= mask;
}

/* 0x8011F240 - raises the hate toward the attacker (player, enemy or other) by the hit's damage. */
extern "C" void em_damage_hate_add(_ENEMY_WORK* self, u8 side, u8 index, s16 damage, u8 heavy)
{
    u32 ally = 0;
    s16 amount = em_damage_bonus_apply(self, damage, heavy, 2);
    s32 hate;
    EmPlayerWork* player;
    EmEnemyRec* other;

    switch (side) {
    case 0:
        hate = amount * 10 + 200;
        player = &((EmPlayerWork*)get_move_work_adrs(2))[index];
        if (player->active != 0) {
            if (Pl_Skill_ck((_PLW*)player, 12) == 1) {
                hate = hate * 1.5f;
            }
            if (hate > 2000) {
                hate = 2000;
            }
            em_hate_pl_add(self, index, hate);
            if (amount > 0) {
                em_hate_add(self, index, 20000);
            }
            em_sight_player_set(self, index);
        }
        break;
    case 1:
        other = &((EmEnemyRec*)get_move_work_adrs(3))[index];
        if (other->active != 0 && (self->team == other->team || em_ally_kind_ck(self, other->team) == 1)) {
            ally = 1;
        }
        hate = amount * 8 + 200;
        if (hate > 2000) {
            hate = 2000;
        }
        em_hate_em_add(self, index, hate);
        break;
    case 2:
        hate = amount * 5 + 100;
        if (hate > 2000) {
            hate = 2000;
        }
        em_hate_misc_add(self, hate);
        break;
    }
    if (ally == 0 && (self->field_0x1C8 & 0x4000) && amount > 0) {
        em_hit_latch_set(self);
    }
}

/* 0x8011F420 - whether a part-limit flinch may start: not in water or lava, not inside the stage's no-flinch zone,
 * and the kind's hook (slot 0xF) agrees. */
extern "C" u32 em_event_settle_kind14_ck(_ENEMY_WORK* self)
{
    u32 zone;
    u32 (*hook)(_ENEMY_WORK*);

    if (em_in_water_ck(self, 1) == 1) {
        return 0;
    }
    if (em_magma_check(self) == 1) {
        return 0;
    }
    if (stage_zone_hit_ck(stage_group_word_get(get_now_mapno(), self->act_id), &zone, &self->pos) == 1) {
        return 0;
    }
    hook = (u32 (*)(_ENEMY_WORK*))em_hook_get(self, 0xF);
    if (hook != NULL) {
        return hook(self);
    }
    return 1;
}

/* 0x8011F4D0 - the kind's hook (slot 0x10) for reaction 16, yes without one. */
extern "C" u32 em_event_settle_kind16_ck(_ENEMY_WORK* self)
{
    u32 (*hook)(_ENEMY_WORK*) = (u32 (*)(_ENEMY_WORK*))em_hook_get(self, 0x10);

    if (hook != NULL) {
        return hook(self);
    }
    return 1;
}

/* 0x8011F520 - the kind's hook (slot 0x12) for reaction 11. */
extern "C" u32 em_event_settle_kind11_ck(_ENEMY_WORK* self)
{
    u32 (*hook)(_ENEMY_WORK*) = (u32 (*)(_ENEMY_WORK*))em_hook_get(self, 0x12);

    if (hook != NULL) {
        return hook(self);
    }
    return 1;
}

/* 0x8011F570 - the kind's hook (slot 0x13) for reaction 19. */
extern "C" u32 em_event_settle_kind19_ck(_ENEMY_WORK* self)
{
    u32 (*hook)(_ENEMY_WORK*) = (u32 (*)(_ENEMY_WORK*))em_hook_get(self, 0x13);

    if (hook != NULL) {
        return hook(self);
    }
    return 1;
}

/* 0x8011F5C0 - re-enters the user-data state when a live, idle enemy's quest entry asks for it (hook slot 0x16). */
extern "C" void em_damage_reenter_ck(_ENEMY_WORK* self)
{
    void (*hook)(_ENEMY_WORK*);

    if (self->field_0x1F8 != 1 && em_busy_ck(self) != 0 && (u32)em_die_ck(self) != 1 && em_quest_entry_live_ck(self) == 1) {
        hook = (void (*)(_ENEMY_WORK*))em_hook_get(self, 0x16);
        if (hook != NULL) {
            hook(self);
        }
        em_user_data_reenter(self);
        em_state_refresh(self);
    }
}

/* 0x8011F654 - settles the frame's damage: part gauges, part limits, ailments and health, then picks the reaction
 * kind (+0x82C) and starts it. */
extern "C" s32 em_damage_eval(_ENEMY_WORK* self)
{
    s32 limit_hit = 0;
    u32 seated = 0;
    s32 struck;
    s32 total;
    s16 most;
    u8 i;
    s16 damage;
    s32 left;
    EmPartLimitRec* rec;
    s16 first;
    s16 second;
    s16 worn;
    s8 slot;
    u8 ailment;

    if (self->hit_flag_0x918 == 1) {
        struck = 1;
        self->hit_flag_0x918 = 0;
    } else {
        struck = 0;
    }
    if (em_busy_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x1F8 != 0) {
        return 0;
    }
    self->part_broken_0x82E = 0;
    self->react_kind_0x82C = 0;
    self->limit_part_0x832 = 0xFF;
    if (em_die_ck(self) == 0) {
        if (self->mask_0x80E & 1) {
            if (self->field_0x834 == 1 && em_status_bits_ck(self, 0x10000) == 1) {
                seated = 1;
            } else if (em_status_bits_ck(self, 0x4000) == 1) {
                switch (self->field_0x835) {
                case 0:
                    self->field_0x834 = 1;
                    break;
                case 1:
                    if (em_act_ball_ck(self, 0) == 1) {
                        self->field_0x834 = 1;
                    }
                    break;
                }
            } else {
                self->field_0x834 = 1;
            }
        }
        total = 0;
        most = 0;
        for (i = 0; i < 8; i++) {
            damage = self->part_damage_0x7CA[i];
            left = self->parts_0x838[i].hp_0x02 - damage;
            total += damage;
            if ((s16)left <= 0) {
                self->part_broken_0x82E |= (u16)(1 << i);
                em_parts_damage_add(self, i, 1);
                em_parts_hp_reset(self, i);
                em_parts_refresh(self, i, 0);
                if (damage > most) {
                    self->react_part_0x831 = i;
                    most = damage;
                }
            } else {
                self->parts_0x838[i].hp_0x02 = left;
            }
            if (!(self->part_mask_0x820 & (1 << i)) || self->part_count_0x821 <= 0) {
                rec = em_part_limit_get(self, i);
                if (rec->state_max_0x00 > self->parts_0x838[i].state_0x01) {
                    first = -1;
                    if (rec->kind_a_0x02 != 0xFF) {
                        slot = em_part_slot_get(self, rec->kind_a_0x02);
                        if (slot != -1) {
                            first = self->kind_damage_0x7DA[slot][i];
                        }
                    }
                    second = -1;
                    if (rec->kind_b_0x03 != 0xFF) {
                        slot = em_part_slot_get(self, rec->kind_b_0x03);
                        if (slot != -1) {
                            second = self->kind_damage_0x7DA[slot][i];
                        }
                    }
                    if (first > 0 && second == -1) {
                        worn = first;
                    } else if (second > 0 && first == -1) {
                        worn = second;
                    } else if (first > 0 && second > 0) {
                        if (first > second) {
                            worn = first;
                        } else {
                            worn = second;
                        }
                    } else {
                        worn = 0;
                    }
                    if (worn > 0) {
                        left = self->parts_0x838[i].limit_0x04 - worn;
                        if ((s16)left <= 0) {
                            if (em_act_trap_ck(self) == 0 && em_act_sleep_ck(self) == 0 &&
                                em_event_settle_kind14_ck(self) == 1 && em_flag_0x8AE_ck(self) == 0 &&
                                (self->state_0x1E2 == 0 || self->state_0x1E2 == 2) && self->limit_part_0x832 == 0xFF) {
                                self->limit_part_0x832 = i;
                            }
                            self->parts_0x838[i].limit_0x04 = 1;
                        } else {
                            self->parts_0x838[i].limit_0x04 = left;
                        }
                    }
                }
            }
        }
        ailment = em_status_buildup_eval(self);
        do {
            if (self->timer_0x81A <= 0 && em_damage_area_ok_ck(self) == 1 && em_flag_0x8AE_ck(self) == 0) {
                if (ailment == 3) {
                    self->react_kind_0x82C = 6;
                    break;
                }
                if (self->hit_0x833 != 0 && (s32)self->hit_kind_0x82A == 12 && em_status_bits_ck(self, 0x80) == 1 &&
                    (self->state_0x1E2 == 0 || self->state_0x1E2 == 2)) {
                    em_gauge_set(self, 0);
                    self->react_kind_0x82C = 15;
                    break;
                }
                if (self->limit_part_0x832 != 0xFF && em_status_bits_ck(self, 0x8000) == 1) {
                    em_gauge_set(self, 0);
                    self->react_kind_0x82C = 20;
                    self->part_hit_0x801 = self->part_hit_0x800;
                    break;
                }
                if (seated == 1) {
                    em_gauge_set(self, 0);
                    self->react_kind_0x82C = 21;
                    break;
                }
            }
            if (self->extra_damage_0x928 != 0) {
                em_gauge_step(self, -self->extra_damage_0x928);
                if ((self->field_0x1C8 & 1) && self->amount_0x7A0 <= 0) {
                    em_gauge_set(self, 1);
                }
            }
            if (total != 0) {
                em_gauge_step(self, -total);
            }
            if ((total != 0 || self->extra_damage_0x928 != 0) && self->amount_0x7A0 <= 0) {
                if (self->timer_0x81A > 0 || em_damage_area_ok_ck(self) == 0) {
                    em_gauge_set(self, 1);
                } else {
                    if (self->extra_damage_0x928 != 0 && em_status_bits_ck(self, 0x200) == 1) {
                        self->react_kind_0x82C = 17;
                    } else {
                        self->react_kind_0x82C = 1;
                    }
                    break;
                }
            }
            self->extra_damage_0x928 = 0;
            do {
                if (self->timer_0x81A > 0 || em_act_kind23_ck(self) == 1) {
                    self->react_kind_0x82C = 0;
                    break;
                }
                if (self->hit_0x833 == 0 && struck == 0 && ailment == 0) {
                    break;
                }
                if (self->hit_0x833 != 0 && em_act_trap_ck(self) == 0) {
                    switch (self->hit_kind_0x82A) {
                    case 10:
                        if (em_status_bits_ck(self, 0x20) == 1) {
                            self->react_kind_0x82C = 12;
                            continue;
                        }
                        break;
                    case 11:
                        if (em_act_paralysis_ck(self) == 0 && em_status_bits_ck(self, 0x40) == 1) {
                            self->react_kind_0x82C = 13;
                            continue;
                        }
                        break;
                    case 34:
                        if (em_act_kind23_ck(self) == 0 && em_status_bits_ck(self, 0x40000) == 1) {
                            self->react_kind_0x82C = 23;
                            continue;
                        }
                        break;
                    case 35:
                        if (em_act_wake_ck(self) == 0 && em_status_bits_ck(self, 0x80000) == 1) {
                            self->react_kind_0x82C = 24;
                            continue;
                        }
                        break;
                    case 16:
                        if (em_act_kind18_ck(self) == 0 && em_status_bits_ck(self, 0x400) == 1) {
                            self->react_kind_0x82C = 18;
                            continue;
                        }
                        break;
                    }
                }
                if (em_act_wake_ck(self) == 1 || em_act_kind18_ck(self) == 1) {
                    self->react_kind_0x82C = 0;
                    break;
                }
                if (self->field_0x7B0 >= (f32)self->amount_0x7A0 / (f32)(s32)self->field_0x7a4 &&
                    em_status_bits_ck(self, 0x400000) == 1) {
                    self->react_kind_0x82C = 26;
                    break;
                }
                if (self->limit_part_0x832 != 0xFF) {
                    em_parts_state_add(self, self->limit_part_0x832, 1);
                    em_parts_limit_reset(self, self->limit_part_0x832);
                    self->react_kind_0x82C = 14;
                    break;
                }
                if (self->hit_0x833 != 0 && em_sleep_ck(self, 1) == 1) {
                    self->react_kind_0x82C = 9;
                    break;
                }
                if (em_status_ck(self, 29) != 0 || em_act_part_flinch_ck(self) != 0) {
                    break;
                }
                if (ailment != 0 && ailment != 5) {
                    switch (ailment) {
                    case 1:
                        self->react_kind_0x82C = 4;
                        break;
                    case 2:
                        self->react_kind_0x82C = 5;
                        break;
                    case 4:
                        self->react_kind_0x82C = 7;
                        break;
                    }
                    break;
                }
                if (em_act_trap_ck(self) != 0 || em_act_paralysis_ck(self) != 0 || em_act_sleep_ck(self) != 0 ||
                    em_act_stun_ck(self) != 0) {
                    break;
                }
                if (self->hit_0x833 != 0 &&
                    ((u32)(self->hit_kind_0x82A - 4) <= 1 || (u32)(self->hit_kind_0x82A - 35) <= 1) &&
                    em_status_bits_ck(self, 0x10) == 1) {
                    if (em_event_settle_kind11_ck(self) == 1) {
                        self->react_kind_0x82C = 11;
                        break;
                    }
                    if (total == 0) {
                        limit_hit = 1;
                    }
                }
                if (struck != 0 && em_status_bits_ck(self, 4) == 1) {
                    self->react_kind_0x82C = 10;
                    break;
                }
                if (ailment == 5 && em_flag_0x8AE_ck(self) == 0 &&
                    (self->action_0x1E5 != 10 || em_act_flinch_light_ck(self) == 1)) {
                    self->react_kind_0x82C = 8;
                    break;
                }
                if ((self->action_0x1E5 != 10 || em_act_flinch_light_ck(self) == 1) && self->part_broken_0x82E != 0 &&
                    em_status_bits_ck(self, 2) == 1) {
                    self->react_kind_0x82C = 3;
                    break;
                }
                if ((u8)(self->action_0x1E5 - 8) <= 1 && self->timer_0x81C > 0 && (self->mask_0x80E & 2)) {
                    self->react_kind_0x82C = 22;
                    break;
                }
                if (self->action_0x1E5 != 10 && (self->mask_0x80E & 2) && em_status_bits_ck(self, 0x100000) == 1) {
                    self->react_kind_0x82C = 25;
                    break;
                }
                if (limit_hit != 0) {
                    break;
                }
                if (self->action_0x1E5 != 10 && self->state_0x1E2 == 0 && em_flag_0x8AE_ck(self) == 0 &&
                    em_status_bits_ck(self, 0x100) == 1 && em_event_settle_kind16_ck(self) == 1) {
                    self->react_kind_0x82C = 16;
                    break;
                }
                if (self->action_0x1E5 != 10 && em_event_settle_kind19_ck(self) == 1 &&
                    em_status_bits_ck(self, 0x4000) == 1 && (self->mask_0x80E & 1)) {
                    self->react_kind_0x82C = 19;
                    break;
                }
                if ((self->action_0x1E5 != 10 || em_act_ball_ck(self, 0) == 1) &&
                    (em_flag_0x8AE_ck(self) == 0 || em_status_bits_ck(self, 0x200000) == 1) &&
                    em_status_bits_ck(self, 1) == 1) {
                    self->react_kind_0x82C = 2;
                }
            } while (0);
                if (total != 0) {
                    em_anger_add(self, em_damage_total_filter(self, total));
                }
        } while (0);
    }
    return em_event_settle_dispatch(self);
}

/* 0x80120198 - turns the frame's ailment build-up into the ailment that fires: 3 for kind 12, 1/2/4 for sleep,
 * paralysis and stun, 5 for exhaustion; poison starts on its own.  0 when none fires. */
extern "C" u8 em_status_buildup_eval(_ENEMY_WORK* self)
{
    s32 sleep = 0;
    s32 paralysis = 0;
    s32 kind12 = 0;
    s32 stun = 0;
    s32 exhaust = 0;

    get_enemy_data(self);
    if (em_poison_add(self, em_status_buildup_get(self, 10)) == 1 && em_status_ck(self, 2) == 0) {
        em_status_start(self);
    }
    if (self->timer_0x81A <= 0) {
        if ((em_sleep_add(self, em_status_buildup_get(self, 9)) == 1 || em_ailment_mask_ck(self, 1) == 1) &&
            em_act_roar_ck(self) == 0) {
            sleep = 1;
        }
        if ((em_paralysis_add(self, em_status_buildup_get(self, 11)) == 1 || em_ailment_mask_ck(self, 4) == 1) &&
            em_act_roar_ck(self) == 0) {
            paralysis = 1;
        }
        if ((em_stun_add(self, self->stun_0x80A) == 1 || em_ailment_mask_ck(self, 0x10) == 1) &&
            em_act_roar_ck(self) == 0) {
            stun = 1;
        }
    }
    if (self->exhaust_0x80C > 0 && em_exhaust_add(self, self->exhaust_0x80C) == 1 && em_act_roar_ck(self) == 0) {
        exhaust = 1;
    }
    if (self->timer_0x81A <= 0 &&
        (em_ailment8_add(self, em_status_buildup_get(self, 12)) == 1 || em_ailment_mask_ck(self, 8) == 1) &&
        em_ailment8_window_ck(self) == 1 && em_act_roar_ck(self) == 0) {
        kind12 = 1;
    }
    if (kind12 != 0) {
        if (em_status_ck(self, 8) == 0) {
            return 3;
        }
    } else if (sleep != 0) {
        if (em_status_ck(self, 29) == 0) {
            return 1;
        }
    } else if (paralysis != 0) {
        if (em_status_ck(self, 29) == 0) {
            return 2;
        }
    } else if (stun != 0) {
        if (em_status_ck(self, 29) == 0) {
            return 4;
        }
    } else if (exhaust != 0 && em_status_ck(self, 29) == 0 && em_act_trap_ck(self) == 0 &&
               em_act_paralysis_ck(self) == 0 && em_act_sleep_ck(self) == 0 &&
               (self->state_0x1E2 == 0 || self->state_0x1E2 == 2)) {
        return 5;
    }
    return 0;
}

/* 0x80120494 - the frame's build-up of ailment `kind` (9..12). */
extern "C" s16 em_status_buildup_get(_ENEMY_WORK* self, u8 kind)
{
    return self->ailment_0x802[kind - 9];
}

/* 0x801204AC - the frame's total damage as the anger gauge takes it. */
extern "C" s16 em_damage_total_filter(_ENEMY_WORK* self, s32 total)
{
    return total;
}

/* 0x801204B4 - how reaction `kind` measures the hit direction (data table, first byte). */
extern "C" u8 em_hit_dir_mode_get(_ENEMY_WORK* self, u8 kind)
{
    return get_enemy_data(self)->hit_dirs_0x34[kind * 2];
}

/* 0x801204F0 - whether reaction `kind` turns the enemy toward the hit (data table, second byte). */
extern "C" u8 em_hit_dir_turn_mode_get(_ENEMY_WORK* self, u8 kind)
{
    return get_enemy_data(self)->hit_dirs_0x34[kind * 2 + 1];
}

/* 0x80120530 - the side the hit came from for reaction `kind` (0 back, 1 left, 2 front, 3 right; 4 = not
 * measured), turning the enemy to face it when the kind asks. */
extern "C" u8 em_hit_dir_get(_ENEMY_WORK* self, u8 kind)
{
    u16 diff = self->pos_0x1BC.y - self->hit_angle_0x816;
    s32 dir;

    switch (em_hit_dir_mode_get(self, kind)) {
    case 1:
        if (diff < 0x8000) {
            dir = 1;
        } else {
            dir = 0;
        }
        break;
    case 2:
        if ((u16)(diff + 0xC000) > 0x7FFF) {
            dir = 2;
        } else {
            dir = 3;
        }
        break;
    case 3:
        if ((u16)(diff + 0xE000) > 0xBFFF) {
            dir = 2;
        } else if (diff < 0x6000) {
            dir = 1;
        } else if (diff < 0xA000) {
            dir = 3;
        } else {
            dir = 0;
        }
        break;
    default:
        dir = 4;
        break;
    }
    switch (em_hit_dir_turn_mode_get(self, kind)) {
    case 1:
        self->pos_0x1BC.y = self->hit_angle_0x816;
        break;
    case 2:
        switch (dir) {
        case 2:
            self->pos_0x1BC.y = self->hit_angle_0x816;
            break;
        case 3:
            self->pos_0x1BC.y = (u16)(self->hit_angle_0x816 + 0x8000);
            break;
        case 1:
            self->pos_0x1BC.y = (u16)(self->hit_angle_0x816 + 0x4000);
            break;
        case 0:
            self->pos_0x1BC.y = (u16)(self->hit_angle_0x816 - 0x4000);
            break;
        }
        break;
    }
    return dir;
}

/* 0x801206C8 - reaction 1 (and 27): the death motions, by trap/sleep/ground state and hit side; `force` and the
 * kill counters pick the second set (+0x8D3 marks it). */
extern "C" s32 em_event_settle(_ENEMY_WORK* self, u8 force)
{
    s32 alt;

    self->field_0x8D3 = 0;
    if (em_status_bits_ck(self, 0x20000) == 1 &&
        (force == 1 || self->field_0x93E >= self->field_0x940 ||
         em_act_leap_ck(self->action_0x1E5, self->state_sub) == 1)) {
        alt = 1;
    } else {
        alt = 0;
    }
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 11, 15);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 11, 52);
            break;
        case 1:
            em_act_request_react(self, 11, 53);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 11, 14);
    } else {
        switch (self->state_0x1E2) {
        case 1:
            if (self->field_0x1E3 == 2) {
                if (alt == 0) {
                    em_act_request_react(self, 11, 12);
                } else {
                    em_act_request_react(self, 11, 49);
                    self->field_0x8D3 = 1;
                }
                break;
            }
            if (self->field_0x1E3 == 1) {
                if (alt == 0) {
                    em_act_request_react(self, 11, 13);
                } else {
                    em_act_request_react(self, 11, 50);
                    self->field_0x8D3 = 1;
                }
                break;
            }
        case 3:
            if (alt == 0) {
                switch (em_hit_dir_get(self, 12)) {
                default:
                    em_act_request_react(self, 11, 5);
                    break;
                case 0:
                    em_act_request_react(self, 11, 6);
                    break;
                case 1:
                    em_act_request_react(self, 11, 7);
                    break;
                case 2:
                    em_act_request_react(self, 11, 8);
                    break;
                case 3:
                    em_act_request_react(self, 11, 9);
                    break;
                }
            } else {
                switch (em_hit_dir_get(self, 12)) {
                default:
                    em_act_request_react(self, 11, 42);
                    break;
                case 0:
                    em_act_request_react(self, 11, 43);
                    break;
                case 1:
                    em_act_request_react(self, 11, 44);
                    break;
                case 2:
                    em_act_request_react(self, 11, 45);
                    break;
                case 3:
                    em_act_request_react(self, 11, 46);
                    break;
                }
                self->field_0x8D3 = 1;
            }
            break;
        case 2:
            if (alt == 0) {
                em_act_request_react(self, 11, 10);
            } else {
                em_act_request_react(self, 11, 47);
                self->field_0x8D3 = 1;
            }
            break;
        case 4:
            if (alt == 0) {
                em_act_request_react(self, 11, 11);
            } else {
                em_act_request_react(self, 11, 48);
                self->field_0x8D3 = 1;
            }
            break;
        default:
            if (alt == 0) {
                switch (em_hit_dir_get(self, 11)) {
                default:
                    em_act_request_react(self, 11, 0);
                    break;
                case 0:
                    em_act_request_react(self, 11, 1);
                    break;
                case 1:
                    em_act_request_react(self, 11, 2);
                    break;
                case 2:
                    em_act_request_react(self, 11, 3);
                    break;
                case 3:
                    em_act_request_react(self, 11, 4);
                    break;
                }
            } else {
                switch (em_hit_dir_get(self, 9)) {
                default:
                    em_act_request_react(self, 11, 37);
                    break;
                case 0:
                    em_act_request_react(self, 11, 38);
                    break;
                case 1:
                    em_act_request_react(self, 11, 39);
                    break;
                case 2:
                    em_act_request_react(self, 11, 40);
                    break;
                case 3:
                    em_act_request_react(self, 11, 41);
                    break;
                }
                self->field_0x8D3 = 1;
            }
            break;
        }
    }
    return 1;
}

/* 0x80120B84 - reaction 2: the plain flinch, by state and hit side. */
extern "C" s32 em_event_settle_kind2(_ENEMY_WORK* self)
{
    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0x12);
        return 1;
    }
    switch (self->state_0x1E2) {
    default:
        switch (em_hit_dir_get(self, 0)) {
        default:
            em_act_request_react(self, 10, 0);
            break;
        case 0:
            em_act_request_react(self, 10, 1);
            break;
        case 1:
            em_act_request_react(self, 10, 2);
            break;
        case 2:
            em_act_request_react(self, 10, 3);
            break;
        case 3:
            em_act_request_react(self, 10, 4);
            break;
        }
        break;
    case 1:
        if (self->field_0x1E3 == 2) {
            em_act_request_react(self, 10, 0x10);
            break;
        }
        if (self->field_0x1E3 == 1) {
            em_act_request_react(self, 10, 0x11);
            break;
        }
    case 3:
        switch (em_hit_dir_get(self, 3)) {
        default:
            em_act_request_react(self, 10, 5);
            break;
        case 0:
            em_act_request_react(self, 10, 6);
            break;
        case 1:
            em_act_request_react(self, 10, 7);
            break;
        case 2:
            em_act_request_react(self, 10, 8);
            break;
        case 3:
            em_act_request_react(self, 10, 9);
            break;
        }
        break;
    case 2:
        switch (em_hit_dir_get(self, 6)) {
        default:
            em_act_request_react(self, 10, 0xA);
            break;
        case 0:
            em_act_request_react(self, 10, 0xB);
            break;
        case 1:
            em_act_request_react(self, 10, 0xC);
            break;
        case 2:
            em_act_request_react(self, 10, 0xD);
            break;
        case 3:
            em_act_request_react(self, 10, 0xE);
            break;
        }
        break;
    case 4:
        em_act_request_react(self, 10, 0xF);
        break;
    }
    return 1;
}

/* 0x80120E00 (0x3D4): reaction 3, the part-break flinch: by state, hit side and the part that broke. */
extern "C" s32 em_event_settle_kind3_default(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    default:
        switch (em_hit_dir_get(self, 1)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x17);
                break;
            case 1:
                em_act_request_react(self, 10, 0x18);
                break;
            case 2:
                em_act_request_react(self, 10, 0x19);
                break;
            case 3:
                em_act_request_react(self, 10, 0x1A);
                break;
            case 4:
                em_act_request_react(self, 10, 0x1B);
                break;
            case 5:
                em_act_request_react(self, 10, 0x1C);
                break;
            case 6:
                em_act_request_react(self, 10, 0x1D);
                break;
            case 7:
                em_act_request_react(self, 10, 0x1E);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x1F);
            break;
        case 1:
            em_act_request_react(self, 10, 0x20);
            break;
        case 2:
            em_act_request_react(self, 10, 0x21);
            break;
        case 3:
            em_act_request_react(self, 10, 0x22);
            break;
        }
        break;
    case 1:
        if (self->field_0x1E3 == 2) {
            em_act_request_react(self, 10, 0x35);
            break;
        }
        if (self->field_0x1E3 == 1) {
            em_act_request_react(self, 10, 0x36);
            break;
        }
    case 3:
        switch (em_hit_dir_get(self, 4)) {
        default:
            em_act_request_react(self, 10, 0x23);
            break;
        case 0:
            em_act_request_react(self, 10, 0x24);
            break;
        case 1:
            em_act_request_react(self, 10, 0x25);
            break;
        case 2:
            em_act_request_react(self, 10, 0x26);
            break;
        case 3:
            em_act_request_react(self, 10, 0x27);
            break;
        }
        break;
    case 2:
        switch (em_hit_dir_get(self, 7)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x28);
                break;
            case 1:
                em_act_request_react(self, 10, 0x29);
                break;
            case 2:
                em_act_request_react(self, 10, 0x2A);
                break;
            case 3:
                em_act_request_react(self, 10, 0x2B);
                break;
            case 4:
                em_act_request_react(self, 10, 0x2C);
                break;
            case 5:
                em_act_request_react(self, 10, 0x2D);
                break;
            case 6:
                em_act_request_react(self, 10, 0x2E);
                break;
            case 7:
                em_act_request_react(self, 10, 0x2F);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x30);
            break;
        case 1:
            em_act_request_react(self, 10, 0x31);
            break;
        case 2:
            em_act_request_react(self, 10, 0x32);
            break;
        case 3:
            em_act_request_react(self, 10, 0x33);
            break;
        }
        break;
    case 4:
        em_act_request_react(self, 10, 0x34);
        break;
    }
    return 1;
}

/* 0x801211D4 (0x3D4): reaction 3 for a kind whose hook slot 0xE claims the part: the second motion set. */
extern "C" s32 em_event_settle_kind3_cb14(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    default:
        switch (em_hit_dir_get(self, 1)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x38);
                break;
            case 1:
                em_act_request_react(self, 10, 0x39);
                break;
            case 2:
                em_act_request_react(self, 10, 0x3A);
                break;
            case 3:
                em_act_request_react(self, 10, 0x3B);
                break;
            case 4:
                em_act_request_react(self, 10, 0x3C);
                break;
            case 5:
                em_act_request_react(self, 10, 0x3D);
                break;
            case 6:
                em_act_request_react(self, 10, 0x3E);
                break;
            case 7:
                em_act_request_react(self, 10, 0x3F);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x40);
            break;
        case 1:
            em_act_request_react(self, 10, 0x41);
            break;
        case 2:
            em_act_request_react(self, 10, 0x42);
            break;
        case 3:
            em_act_request_react(self, 10, 0x43);
            break;
        }
        break;
    case 1:
        if (self->field_0x1E3 == 2) {
            em_act_request_react(self, 10, 0x56);
            break;
        }
        if (self->field_0x1E3 == 1) {
            em_act_request_react(self, 10, 0x57);
            break;
        }
    case 3:
        switch (em_hit_dir_get(self, 4)) {
        default:
            em_act_request_react(self, 10, 0x44);
            break;
        case 0:
            em_act_request_react(self, 10, 0x45);
            break;
        case 1:
            em_act_request_react(self, 10, 0x46);
            break;
        case 2:
            em_act_request_react(self, 10, 0x47);
            break;
        case 3:
            em_act_request_react(self, 10, 0x48);
            break;
        }
        break;
    case 2:
        switch (em_hit_dir_get(self, 7)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x49);
                break;
            case 1:
                em_act_request_react(self, 10, 0x4A);
                break;
            case 2:
                em_act_request_react(self, 10, 0x4B);
                break;
            case 3:
                em_act_request_react(self, 10, 0x4C);
                break;
            case 4:
                em_act_request_react(self, 10, 0x4D);
                break;
            case 5:
                em_act_request_react(self, 10, 0x4E);
                break;
            case 6:
                em_act_request_react(self, 10, 0x4F);
                break;
            case 7:
                em_act_request_react(self, 10, 0x50);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x51);
            break;
        case 1:
            em_act_request_react(self, 10, 0x52);
            break;
        case 2:
            em_act_request_react(self, 10, 0x53);
            break;
        case 3:
            em_act_request_react(self, 10, 0x54);
            break;
        }
        break;
    case 4:
        em_act_request_react(self, 10, 0x55);
        break;
    }
    return 1;
}

/* 0x801215A8 (0x3D4): reaction 3 for a kind whose hook slot 0x11 claims the part: the third motion set. */
extern "C" s32 em_event_settle_kind3_cb17(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    default:
        switch (em_hit_dir_get(self, 2)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x58);
                break;
            case 1:
                em_act_request_react(self, 10, 0x59);
                break;
            case 2:
                em_act_request_react(self, 10, 0x5A);
                break;
            case 3:
                em_act_request_react(self, 10, 0x5B);
                break;
            case 4:
                em_act_request_react(self, 10, 0x5C);
                break;
            case 5:
                em_act_request_react(self, 10, 0x5D);
                break;
            case 6:
                em_act_request_react(self, 10, 0x5E);
                break;
            case 7:
                em_act_request_react(self, 10, 0x5F);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x60);
            break;
        case 1:
            em_act_request_react(self, 10, 0x61);
            break;
        case 2:
            em_act_request_react(self, 10, 0x62);
            break;
        case 3:
            em_act_request_react(self, 10, 0x63);
            break;
        }
        break;
    case 1:
        if (self->field_0x1E3 == 2) {
            em_act_request_react(self, 10, 0x76);
            break;
        }
        if (self->field_0x1E3 == 1) {
            em_act_request_react(self, 10, 0x77);
            break;
        }
    case 3:
        switch (em_hit_dir_get(self, 5)) {
        default:
            em_act_request_react(self, 10, 0x64);
            break;
        case 0:
            em_act_request_react(self, 10, 0x65);
            break;
        case 1:
            em_act_request_react(self, 10, 0x66);
            break;
        case 2:
            em_act_request_react(self, 10, 0x67);
            break;
        case 3:
            em_act_request_react(self, 10, 0x68);
            break;
        }
        break;
    case 2:
        switch (em_hit_dir_get(self, 8)) {
        default:
            switch (self->react_part_0x831) {
            default:
                em_act_request_react(self, 10, 0x69);
                break;
            case 1:
                em_act_request_react(self, 10, 0x6A);
                break;
            case 2:
                em_act_request_react(self, 10, 0x6B);
                break;
            case 3:
                em_act_request_react(self, 10, 0x6C);
                break;
            case 4:
                em_act_request_react(self, 10, 0x6D);
                break;
            case 5:
                em_act_request_react(self, 10, 0x6E);
                break;
            case 6:
                em_act_request_react(self, 10, 0x6F);
                break;
            case 7:
                em_act_request_react(self, 10, 0x70);
                break;
            }
            break;
        case 0:
            em_act_request_react(self, 10, 0x71);
            break;
        case 1:
            em_act_request_react(self, 10, 0x72);
            break;
        case 2:
            em_act_request_react(self, 10, 0x73);
            break;
        case 3:
            em_act_request_react(self, 10, 0x74);
            break;
        }
        break;
    case 4:
        em_act_request_react(self, 10, 0x75);
        break;
    }
    return 1;
}


/* 0x8012197C - reaction 3: the part-break flinch, in the motion set the kind's hooks (slots 0x11, 0xE) choose for
 * the broken part. */
extern "C" s32 em_event_settle_kind3(_ENEMY_WORK* self)
{
    u32 (*hook)(_ENEMY_WORK*, u8);
    u32 claimed;

    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0x37);
        return 1;
    }
    hook = (u32 (*)(_ENEMY_WORK*, u8))em_hook_get(self, 0x11);
    if (hook != NULL) {
        claimed = hook(self, self->react_part_0x831);
    } else {
        claimed = 0;
    }
    if (claimed != 0) {
        return em_event_settle_kind3_cb17(self);
    }
    hook = (u32 (*)(_ENEMY_WORK*, u8))em_hook_get(self, 0xE);
    if (hook != NULL) {
        claimed = hook(self, self->react_part_0x831);
    } else {
        claimed = 0;
    }
    if (claimed != 0) {
        return em_event_settle_kind3_cb14(self);
    }
    return em_event_settle_kind3_default(self);
}

/* 0x80121A58 - the ailment reactions' airborne/swimming motions (state 1/3): arms the ailment mask, then by depth
 * and hit side. */
extern "C" void em_event_settle_status_mode1(_ENEMY_WORK* self, u8 ailment)
{
    em_ailment_mask_set(self, ailment);
    if (self->field_0x1E3 == 2) {
        em_act_request_react(self, 10, 0x98);
        return;
    }
    if (self->field_0x1E3 == 1) {
        em_act_request_react(self, 10, 0x99);
        return;
    }
    switch (em_hit_dir_get(self, 4)) {
    default:
        em_act_request_react(self, 10, 0x84);
        break;
    case 0:
        em_act_request_react(self, 10, 0x85);
        break;
    case 1:
        em_act_request_react(self, 10, 0x86);
        break;
    case 2:
        em_act_request_react(self, 10, 0x87);
        break;
    case 3:
        em_act_request_react(self, 10, 0x88);
        break;
    }
}

/* 0x80121B54 - the ailment reactions' state-4 motion: arms the ailment mask. */
extern "C" void em_event_settle_status_mode4(_ENEMY_WORK* self, u8 ailment)
{
    em_ailment_mask_set(self, ailment);
    em_act_request_react(self, 10, 0x97);
}

/* 0x80121B94 - reaction 4: sleep. */
extern "C" s32 em_event_settle_kind4(_ENEMY_WORK* self)
{
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 10, 0xB9);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 10, 0xE6);
            break;
        case 1:
            em_act_request_react(self, 10, 0xEC);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        if (em_status_bits_ck(self, 0x2000) == 1) {
            em_act_request_react(self, 10, 0x9A);
        } else {
            em_ailment_mask_set(self, 1);
            em_act_request_react(self, 10, 0x9E);
        }
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 10, 0x7A);
            break;
        case 1:
        case 3:
            if (em_status_bits_ck(self, 0x800) == 1) {
                em_act_request_react(self, 10, 0x80);
            } else {
                em_event_settle_status_mode1(self, 1);
            }
            break;
        case 2:
            em_act_request_react(self, 10, 0x89);
            break;
        case 4:
            if (em_status_bits_ck(self, 0x1000) == 1) {
                em_act_request_react(self, 10, 0x93);
            } else {
                em_event_settle_status_mode4(self, 1);
            }
            break;
        }
    }
    return 1;
}

/* 0x80121D50 - reaction 5: paralysis. */
extern "C" s32 em_event_settle_kind5(_ENEMY_WORK* self)
{
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 10, 0xBB);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 10, 0xE8);
            break;
        case 1:
            em_act_request_react(self, 10, 0xEE);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        if (em_status_bits_ck(self, 0x2000) == 1) {
            em_act_request_react(self, 10, 0x9C);
        } else {
            em_ailment_mask_set(self, 4);
            em_act_request_react(self, 10, 0x9E);
        }
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 10, 0x7C);
            break;
        case 1:
        case 3:
            if (em_status_bits_ck(self, 0x800) == 1) {
                em_act_request_react(self, 10, 0x82);
            } else {
                em_event_settle_status_mode1(self, 4);
            }
            break;
        case 2:
            em_act_request_react(self, 10, 0x8B);
            break;
        case 4:
            if (em_status_bits_ck(self, 0x1000) == 1) {
                em_act_request_react(self, 10, 0x95);
            } else {
                em_event_settle_status_mode4(self, 4);
            }
            break;
        }
    }
    return 1;
}

/* 0x80121F0C - reaction 6: the ailment-12 collapse, on the ground or under water. */
extern "C" s32 em_event_settle_kind6(_ENEMY_WORK* self)
{
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 11, 0x1C);
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 11, 0x1A);
            break;
        case 2:
            em_act_request_react(self, 11, 0x1B);
            break;
        case 1:
        case 3:
            return 0;
        }
    }
    return 1;
}

/* 0x80121FA4 - reaction 7: stun. */
extern "C" s32 em_event_settle_kind7(_ENEMY_WORK* self)
{
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 10, 0xBC);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 10, 0xE9);
            break;
        case 1:
            em_act_request_react(self, 10, 0xEF);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        if (em_status_bits_ck(self, 0x2000) == 1) {
            em_act_request_react(self, 10, 0x9D);
        } else {
            em_ailment_mask_set(self, 0x10);
            em_act_request_react(self, 10, 0x9E);
        }
    } else {
        switch (self->state_0x1E2) {
        default:
            switch (em_hit_dir_get(self, 9)) {
            default:
                em_act_request_react(self, 10, 0x7D);
                break;
            case 0:
                em_act_request_react(self, 10, 0x7E);
                break;
            case 1:
                em_act_request_react(self, 10, 0x7F);
                break;
            }
            break;
        case 1:
        case 3:
            if (em_status_bits_ck(self, 0x800) == 1) {
                em_act_request_react(self, 10, 0x83);
            } else {
                em_event_settle_status_mode1(self, 0x10);
            }
            break;
        case 2:
            em_act_request_react(self, 10, 0x8C);
            break;
        case 4:
            if (em_status_bits_ck(self, 0x1000) == 1) {
                em_act_request_react(self, 10, 0x96);
            } else {
                em_event_settle_status_mode4(self, 0x10);
            }
            break;
        }
    }
    return 1;
}

/* 0x801221A8 - reaction 8: exhaustion (ground and water only). */
extern "C" s32 em_event_settle_kind8(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    case 0:
        em_act_request_react(self, 10, 0x78);
        break;
    case 2:
        em_act_request_react(self, 10, 0x79);
        break;
    default:
        return 0;
    }
    return 1;
}

/* 0x80122208 - reaction 9: a hit on a sleeping enemy wakes it. */
extern "C" s32 em_event_settle_kind9(_ENEMY_WORK* self)
{
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 10, 0xB7);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 10, 0xE4);
            break;
        case 1:
            em_act_request_react(self, 10, 0xEA);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0xA7);
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 10, 0xA0);
            break;
        case 1:
            if (self->field_0x1E3 == 2) {
                em_act_request_react(self, 10, 0xA4);
                break;
            }
            if (self->field_0x1E3 == 1) {
                em_act_request_react(self, 10, 0xA5);
                break;
            }
        case 3:
            return 0;
        case 4:
            em_act_request_react(self, 10, 0xA3);
            break;
        case 2:
            em_act_request_react(self, 10, 0xA2);
            break;
        }
    }
    return 1;
}

/* 0x80122378 - reaction 10. */
extern "C" s32 em_event_settle_kind10(_ENEMY_WORK* self)
{
    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0xAE);
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 10, 0xA8);
            break;
        case 1:
            if (self->field_0x1E3 == 2) {
                em_act_request_react(self, 10, 0xAC);
                break;
            }
            if (self->field_0x1E3 == 1) {
                em_act_request_react(self, 10, 0xAD);
                break;
            }
        case 3:
            em_act_request_react(self, 10, 0xA9);
            break;
        case 2:
            em_act_request_react(self, 10, 0xAA);
            break;
        case 4:
            em_act_request_react(self, 10, 0xAB);
            break;
        }
    }
    return 1;
}

/* 0x80122470 - reaction 11. */
extern "C" s32 em_event_settle_kind11(_ENEMY_WORK* self)
{
    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0xB5);
    } else {
        switch (self->state_0x1E2) {
        default:
            em_act_request_react(self, 10, 0xAF);
            break;
        case 1:
            if (self->field_0x1E3 == 2) {
                em_act_request_react(self, 10, 0xB3);
                break;
            }
            if (self->field_0x1E3 == 1) {
                em_act_request_react(self, 10, 0xB4);
                break;
            }
        case 3:
            em_act_request_react(self, 10, 0xB0);
            break;
        case 2:
            em_act_request_react(self, 10, 0xB1);
            break;
        case 4:
            em_act_request_react(self, 10, 0xB2);
            break;
        }
    }
    return 1;
}

/* 0x80122568 - reaction 12: the trap catch. */
extern "C" s32 em_event_settle_kind12(_ENEMY_WORK* self)
{
    em_status_set(self, 1);
    em_act_request_react(self, 10, 0xB6);
    return 1;
}

/* 0x801225AC - reaction 13: the paralysis trap, under water or not. */
extern "C" s32 em_event_settle_kind13(_ENEMY_WORK* self)
{
    if ((s32)self->state_0x1E2 != 2) {
        em_status_set(self, 1);
        em_act_request_react(self, 10, 0xBF);
    } else {
        em_status_set(self, 1);
        em_act_request_react(self, 10, 0xC0);
    }
    return 1;
}

/* 0x80122618 - reaction 14: the part-limit flinch of the part whose limit ran out. */
extern "C" s32 em_event_settle_kind14(_ENEMY_WORK* self)
{
    if (self->state_0x1E2 == 2) {
        switch (self->limit_part_0x832) {
        case 0:
            em_act_request_react(self, 10, 0xCB);
            break;
        case 1:
            em_act_request_react(self, 10, 0xCC);
            break;
        case 2:
            em_act_request_react(self, 10, 0xCD);
            break;
        case 3:
            em_act_request_react(self, 10, 0xCE);
            break;
        case 4:
            em_act_request_react(self, 10, 0xCF);
            break;
        case 5:
            em_act_request_react(self, 10, 0xD0);
            break;
        case 6:
            em_act_request_react(self, 10, 0xD1);
            break;
        case 7:
            em_act_request_react(self, 10, 0xD2);
            break;
        default:
            return 0;
        }
    } else {
        switch (self->limit_part_0x832) {
        case 0:
            em_act_request_react(self, 10, 0xC3);
            break;
        case 1:
            em_act_request_react(self, 10, 0xC4);
            break;
        case 2:
            em_act_request_react(self, 10, 0xC5);
            break;
        case 3:
            em_act_request_react(self, 10, 0xC6);
            break;
        case 4:
            em_act_request_react(self, 10, 0xC7);
            break;
        case 5:
            em_act_request_react(self, 10, 0xC8);
            break;
        case 6:
            em_act_request_react(self, 10, 0xC9);
            break;
        case 7:
            em_act_request_react(self, 10, 0xCA);
            break;
        default:
            return 0;
        }
    }
    return 1;
}

/* 0x8012279C - reaction 15: the knock-down. */
extern "C" s32 em_event_settle_kind15(_ENEMY_WORK* self)
{
    if (self->state_0x1E2 == 2) {
        em_act_request_react(self, 11, 0x13);
    } else {
        switch (em_hit_dir_get(self, 13)) {
        default:
            em_act_request_react(self, 11, 0x10);
            break;
        case 0:
            em_act_request_react(self, 11, 0x11);
            break;
        case 1:
            em_act_request_react(self, 11, 0x12);
            break;
        }
    }
    return 1;
}

/* 0x80122838 - reaction 16. */
extern "C" s32 em_event_settle_kind16(_ENEMY_WORK* self)
{
    switch (em_hit_dir_get(self, 10)) {
    default:
        em_act_request_react(self, 10, 0xD3);
        break;
    case 0:
        em_act_request_react(self, 10, 0xD4);
        break;
    case 1:
        em_act_request_react(self, 10, 0xD5);
        break;
    }
    return 1;
}

/* 0x801228B8 - reaction 17: death by an outside source, by trap/sleep/ground state and hit side. */
extern "C" s32 em_event_settle_kind17(_ENEMY_WORK* self)
{
    s32 alt;

    self->field_0x8D3 = 0;
    if (em_status_bits_ck(self, 0x20000) == 1 &&
        (self->field_0x93E >= self->field_0x940 || em_act_leap_ck(self->action_0x1E5, self->state_sub) == 1)) {
        alt = 1;
    } else {
        alt = 0;
    }
    if (em_act_trap_ck(self) == 1) {
        em_act_request_react(self, 11, 0x24);
    } else if (em_act_sleep_ck(self) == 1) {
        switch ((u8)em_act_sleep_side_get(self)) {
        case 0:
            em_act_request_react(self, 11, 0x36);
            break;
        case 1:
            em_act_request_react(self, 11, 0x37);
            break;
        default:
            return 0;
        }
    } else if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 11, 0x23);
    } else {
        switch (self->state_0x1E2) {
        case 1:
            if (self->field_0x1E3 == 2) {
                if (alt == 0) {
                    em_act_request_react(self, 11, 0x21);
                } else {
                    em_act_request_react(self, 11, 0x31);
                    self->field_0x8D3 = 1;
                }
                break;
            }
            if (self->field_0x1E3 == 1) {
                if (alt == 0) {
                    em_act_request_react(self, 11, 0x22);
                } else {
                    em_act_request_react(self, 11, 0x32);
                    self->field_0x8D3 = 1;
                }
                break;
            }
        case 3:
            if (alt == 0) {
                em_act_request_react(self, 11, 0x1E);
            } else {
                switch (em_hit_dir_get(self, 12)) {
                default:
                    em_act_request_react(self, 11, 0x2A);
                    break;
                case 0:
                    em_act_request_react(self, 11, 0x2B);
                    break;
                case 1:
                    em_act_request_react(self, 11, 0x2C);
                    break;
                case 2:
                    em_act_request_react(self, 11, 0x2D);
                    break;
                case 3:
                    em_act_request_react(self, 11, 0x2E);
                    break;
                }
                self->field_0x8D3 = 1;
            }
            break;
        case 2:
            if (alt == 0) {
                em_act_request_react(self, 11, 0x1F);
            } else {
                em_act_request_react(self, 11, 0x2F);
                self->field_0x8D3 = 1;
            }
            break;
        case 4:
            if (alt == 0) {
                em_act_request_react(self, 11, 0x20);
            } else {
                em_act_request_react(self, 11, 0x30);
                self->field_0x8D3 = 1;
            }
            break;
        default:
            if (alt == 0) {
                em_act_request_react(self, 11, 0x1D);
            } else {
                switch (em_hit_dir_get(self, 9)) {
                default:
                    em_act_request_react(self, 11, 0x25);
                    break;
                case 0:
                    em_act_request_react(self, 11, 0x26);
                    break;
                case 1:
                    em_act_request_react(self, 11, 0x27);
                    break;
                case 2:
                    em_act_request_react(self, 11, 0x28);
                    break;
                case 3:
                    em_act_request_react(self, 11, 0x29);
                    break;
                }
                self->field_0x8D3 = 1;
            }
            break;
        }
    }
    return 1;
}

/* 0x80122C64 - reaction 18. */
extern "C" s32 em_event_settle_kind18(_ENEMY_WORK* self)
{
    if ((s32)self->state_0x1E2 != 4) {
        em_act_request_react(self, 10, 0xBD);
    } else {
        em_act_request_react(self, 10, 0xBE);
    }
    return 1;
}

/* 0x80122CAC - reaction 19. */
extern "C" s32 em_event_settle_kind19(_ENEMY_WORK* self)
{
    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0xDC);
    } else {
        switch (self->state_0x1E2) {
        case 1:
            if (self->field_0x1E3 == 2) {
                em_act_request_react(self, 10, 0xDA);
                break;
            }
            if (self->field_0x1E3 == 1) {
                em_act_request_react(self, 10, 0xDB);
                break;
            }
        case 3:
            em_act_request_react(self, 10, 0xD7);
            break;
        case 2:
            em_act_request_react(self, 10, 0xD8);
            break;
        case 4:
            em_act_request_react(self, 10, 0xD9);
            break;
        default:
            em_act_request_react(self, 10, 0xD6);
            break;
        }
    }
    return 1;
}

/* 0x80122DA8 - reaction 20: the part-limit collapse. */
extern "C" s32 em_event_settle_kind20(_ENEMY_WORK* self)
{
    em_act_request_react(self, 11, 0x33);
    return 1;
}

/* 0x80122DD4 - reaction 21: the seated collapse. */
extern "C" s32 em_event_settle_kind21(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    case 1:
        if (self->field_0x1E3 == 2) {
            em_act_request_react(self, 11, 0x18);
            break;
        }
        if (self->field_0x1E3 == 1) {
            em_act_request_react(self, 11, 0x19);
            break;
        }
    case 3:
        em_act_request_react(self, 11, 0x15);
        break;
    case 2:
        em_act_request_react(self, 11, 0x16);
        break;
    case 4:
        em_act_request_react(self, 11, 0x17);
        break;
    default:
        em_act_request_react(self, 11, 0x14);
        break;
    }
    return 1;
}

/* 0x80122E8C - reaction 22. */
extern "C" s32 em_event_settle_kind22(_ENEMY_WORK* self)
{
    em_act_request_react(self, 10, 0xF0);
    return 1;
}

/* 0x80122EB8 - reaction 23 (ground and state 4 only). */
extern "C" s32 em_event_settle_kind23(_ENEMY_WORK* self)
{
    switch (self->state_0x1E2) {
    case 0:
        em_act_request_react(self, 10, 0xF1);
        return 1;
    case 4:
        em_act_request_react(self, 10, 0xF2);
        return 1;
    default:
        return 0;
    }
}

/* 0x80122F18 - reaction 24: the wake-up, its second pair while +0x81E runs. */
extern "C" s32 em_event_settle_kind24(_ENEMY_WORK* self)
{
    if (self->timer_0x81E > 0) {
        switch (self->state_0x1E2) {
        case 0:
            em_act_request_react(self, 10, 0xF4);
            return 1;
        case 4:
            em_act_request_react(self, 10, 0xF6);
            return 1;
        }
    } else {
        switch (self->state_0x1E2) {
        case 0:
            em_act_request_react(self, 10, 0xF3);
            return 1;
        case 4:
            em_act_request_react(self, 10, 0xF5);
            return 1;
        }
    }
    return 0;
}

/* 0x80122FC4 - reaction 25. */
extern "C" s32 em_event_settle_kind25(_ENEMY_WORK* self)
{
    if (em_flag_0x8AE_ck(self) == 1) {
        em_act_request_react(self, 10, 0xE3);
    } else {
        switch (self->state_0x1E2) {
        case 1:
            if (self->field_0x1E3 == 2) {
                em_act_request_react(self, 10, 0xE1);
                break;
            }
            if (self->field_0x1E3 == 1) {
                em_act_request_react(self, 10, 0xE2);
                break;
            }
        case 3:
            em_act_request_react(self, 10, 0xDE);
            break;
        case 2:
            em_act_request_react(self, 10, 0xDF);
            break;
        case 4:
            em_act_request_react(self, 10, 0xE0);
            break;
        default:
            em_act_request_react(self, 10, 0xDD);
            break;
        }
    }
    return 1;
}

/* 0x801230C0 - reaction 26. */
extern "C" s32 em_event_settle_kind26(_ENEMY_WORK* self)
{
    em_act_request_react(self, 10, 0xF7);
    return 1;
}

/* 0x801230EC - starts the reaction `react_kind_0x82C` names; 0 when there is none. */
extern "C" s32 em_event_settle_dispatch(_ENEMY_WORK* self)
{
    switch (self->react_kind_0x82C) {
    default:
        return 0;
    case 1:
        return em_event_settle(self, 0);
    case 2:
        return em_event_settle_kind2(self);
    case 3:
        return em_event_settle_kind3(self);
    case 4:
        return em_event_settle_kind4(self);
    case 5:
        return em_event_settle_kind5(self);
    case 6:
        return em_event_settle_kind6(self);
    case 7:
        return em_event_settle_kind7(self);
    case 8:
        return em_event_settle_kind8(self);
    case 9:
        return em_event_settle_kind9(self);
    case 10:
        return em_event_settle_kind10(self);
    case 11:
        return em_event_settle_kind11(self);
    case 12:
        return em_event_settle_kind12(self);
    case 13:
        return em_event_settle_kind13(self);
    case 14:
        return em_event_settle_kind14(self);
    case 15:
        return em_event_settle_kind15(self);
    case 16:
        return em_event_settle_kind16(self);
    case 17:
        return em_event_settle_kind17(self);
    case 18:
        return em_event_settle_kind18(self);
    case 19:
        return em_event_settle_kind19(self);
    case 20:
        return em_event_settle_kind20(self);
    case 21:
        return em_event_settle_kind21(self);
    case 22:
        return em_event_settle_kind22(self);
    case 23:
        return em_event_settle_kind23(self);
    case 24:
        return em_event_settle_kind24(self);
    case 25:
        return em_event_settle_kind25(self);
    case 26:
        return em_event_settle_kind26(self);
    case 27:
        return em_event_settle(self, 1);
    }
}

/* 0x80123190 - whether kill-report table entry `sub` names `kind`. */
extern "C" u32 em_kill_sub_kind_ck(u8 sub, u8 kind)
{
    return em_kill_sub_kind_tbl[sub] == kind;
}

/* 0x801231B4 - binds the carcass's roster effect: kinds 20/24/25 their own, the others by whether the enemy lies in
 * water. */
extern "C" void em_roster_default_effect_set(_ENEMY_WORK* self)
{
    switch (self->team) {
    case 20:
        em_roster_slot_effect_set(self, em_roster_record_result_get((EmPopRec*)self), &self->roster_slot_0x888, 8, 0);
        break;
    case 24:
        em_roster_slot_effect_set(self, em_roster_record_result_get((EmPopRec*)self), &self->roster_slot_0x888, 4, 0);
        break;
    case 25:
        em025_effect_slots_reset(self);
        break;
    default:
        if (((u32 (*)(_ENEMY_WORK*))em_water_check)(self) == 1) {
            em_roster_slot_effect_set(self, em_roster_record_result_get((EmPopRec*)self), &self->roster_slot_0x888, 0,
                                      0);
        } else {
            em_roster_slot_effect_set(self, em_roster_record_result_get((EmPopRec*)self), &self->roster_slot_0x888, 1,
                                      0);
        }
        break;
    }
}

/* 0x80123294 - charges the stamina an action step costs: 15, 50 or 200 by the step's motion. */
extern "C" void em_seq_stamina_cost(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34:
    case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50: case 51:
    case 52: case 53: case 54: case 55:
    case 73: case 74: case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82: case 83: case 84:
    case 85: case 86: case 87: case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96:
    case 97: case 98: case 99:
    case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114: case 115:
    case 116: case 117: case 118: case 119:
        em_stamina_add(self, -15);
        break;
    case 35: case 36: case 37: case 38: case 39:
    case 68: case 69: case 70: case 71: case 72:
    case 100: case 101: case 102: case 103: case 104:
    case 128:
    case 130: case 131: case 132: case 133: case 134: case 135: case 136:
    case 147:
    case 149: case 150: case 151: case 152: case 153: case 154:
    case 156: case 157: case 158:
    case 169:
    case 172: case 173:
    case 176:
    case 179: case 180:
    case 182:
    case 195: case 196: case 197: case 198: case 199: case 200: case 201: case 202: case 203: case 204: case 205:
    case 206: case 207: case 208: case 209: case 210:
        em_stamina_add(self, -50);
        break;
    case 120: case 121:
        em_stamina_add(self, -200);
        break;
    }
}

/* 0x801233C8 - loads the move parameters of the script's current motion record into the move vectors. */
extern "C" void em_seq_move_setup(_ENEMY_WORK* self)
{
    EmSeqMotion* motion = self->seq_mot_0x8E0;
    EmSeqMove* move;
    f32* range;

    switch (motion->move_kind) {
    case 1:
        range = motion->range;
        self->v_0x310.x = 0.0f;
        if (self->v_0x310.y < range[0]) {
            self->v_0x310.y = range[0];
        }
        self->v_0x310.z = 0.0f;
        self->v_0x31C.x = 0.0f;
        self->v_0x31C.y = range[1];
        self->v_0x31C.z = 0.0f;
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        move = motion->move;
        if (move != NULL) {
            em_move_vec2_clr(self);
            copyVec3(&self->v_0x310, &move->velocity);
            copyVec3(&self->v_0x31C, &move->accel);
            if (self->seq_mot_0x8E0->move_kind == 3) {
                vec3_scale_in_place(&self->v_0x310, get_em_chg_scale(self));
                vec3_scale_in_place(&self->v_0x31C, get_em_chg_scale(self));
            }
            self->seq_rot_0x8F8 = self->pos_0x1BC.y;
        }
        break;
    case 4:
        range = motion->range;
        em_move_vec_clr(self);
        self->v_0x310.y = (self->field_0x20C - self->pos.y) / range[1];
        if (self->v_0x310.y >= range[0]) {
            self->v_0x310.y = range[0];
        }
        break;
    case 5:
        move = motion->move;
        if (move != NULL) {
            em_move_vec2_clr(self);
            copyVec3(&self->v_0x310, &move->velocity);
            copyVec3(&self->v_0x31C, &move->accel);
            self->seq_rot_0x8F8 = self->pos_0x1BC.y;
        }
        break;
    }
}

/* 0x80123540 - the index of the first record from `index` whose code is `code` (stopping at an end mark). */
extern "C" u8 em_seq_label_find(EmSeqCmd* seq, u8 index, u8 code)
{
    EmSeqCmd* cmd = &seq[index];

    while (cmd->code != code) {
        index++;
        cmd = &seq[index];
        if (cmd->code >= 0xFE) {
            break;
        }
    }
    return index;
}

/* 0x80123584 - tests a script condition; true moves the script to the condition's target table. */
extern "C" u32 em_seq_cond_ck(_ENEMY_WORK* self, EmSeqCond* cond)
{
    u32 hit = 0;
    s32 period;

    switch (cond->kind) {
    case 0:
        if (em_mot_no_ck(self, (u16)cond->value) == 1) {
            hit = 1;
        }
        break;
    case 1:
        if (self->pos.y <= self->field_0x210) {
            hit = 1;
        }
        break;
    case 2:
        period = cond->value;
        if (period == 0) {
            period = 1;
        }
        if (self->bits_0x1EC % period == 0) {
            hit = 1;
        }
        break;
    case 3:
        if (em_break_state_ck(self) == 1) {
            hit = 1;
        }
        break;
    case 4:
        if ((u8)em_hook9_call(self, cond->value) != 0) {
            hit = 1;
        }
        break;
    case 5:
        if (self->joint_flags & (u16)cond->value) {
            hit = 1;
        }
        break;
    }
    if (hit == 1) {
        self->seq_0x8D4 = cond->target;
        self->seq_index_0x8D0 = 0;
    }
    return hit;
}

/* 0x801236D0 - runs the action script from its current record until a motion record starts (0), the script ends
 * (1) or hands the enemy to another state (2). */
extern "C" u8 em_seq_exec(_ENEMY_WORK* self, EmSeqCmd* seq, u8 reverse)
{
    EmSeqCmd* cmd = &seq[self->seq_index_0x8D0];
    EmSeqMotion* prev;
    EmSeqMotion* motion;
    EmSeqMove* move;
    EmSeqState* state;
    EmSeqRoster* roster;
    s32 speed;
    EmSeqTurn* turn;
    f32* floats;
    u32 mode;
    f32 jitter_x;
    f32 jitter_y;
    f32 jitter_z;

    if (self->alpha_0x1D4 <= 0.0f) {
        return 1;
    }
    while (cmd->code != 0xFF) {
        self->seq_index_0x8D0++;
        switch (cmd->code) {
        case 0:
        case 1:
        case 2:
            prev = self->seq_mot_0x8E0;
            self->seq_mot_0x8E0 = cmd->motion;
            if (prev != NULL && prev->flags == 2 && self->seq_mot_0x8E0->flags == 2) {
                break;
            }
            if (cmd->code == 0 || cmd->code == 2) {
                motion = self->seq_mot_0x8E0;
                speed = motion->blend;
                if ((self->field_0x1C8 & 8) && reverse != 0) {
                    speed = -speed;
                }
                em_mot_set_blend(self, motion->motion, speed, motion->frame, motion->loop);
                if (self->seq_speed_0x900 > 0.0f) {
                    em_mot_speed_set(self, self->seq_speed_0x900);
                }
                if (cmd->code == 2) {
                    self->seq_flags_0x8D1 |= 1;
                } else {
                    self->seq_flags_0x8D1 &= ~1;
                }
            }
            em_seq_move_setup(self);
            return 0;
        case 3:
            self->seq_timer_0x20 = cmd->value;
            break;
        case 4:
            if (cmd->count == 1) {
                em_fall_height_get(self);
                em_fall_start(self);
            } else {
                em_move_mode_set(self, (u8)cmd->count);
            }
            break;
        case 5:
            self->seq_frame_0x8E8 = cmd->count;
            break;
        case 6:
            self->seq_flags_0x8D1 |= 2;
            em_flag_0x116_set(self);
            break;
        case 7:
            self->seq_flags_0x8D1 |= 4;
            em_flag_0x115_set(self);
            break;
        case 8:
            seq = cmd->jump;
            self->seq_0x8D4 = seq;
            self->seq_index_0x8D0 = 0;
            break;
        case 9:
            if (em_seq_cond_ck(self, cmd->cond) == 1) {
                seq = self->seq_0x8D4;
            }
            break;
        case 10:
            turn = cmd->turn;
            self->seq_turn_rate_0x8EC = turn->rate;
            self->seq_turn_0x8F0 = turn->window;
            self->seq_turn_angle_0x8F4 = turn->angle;
            break;
        case 11:
            self->seq_ground_0x8FC = cmd->count;
            break;
        case 12:
            self->seq_flags_0x8D1 |= 8;
            em_frame_flag_set(self);
            break;
        case 13:
            em_motion_param_set(self, 0.0f, (s16)cmd->value);
            break;
        case 14:
            self->seq_speed_0x900 = *cmd->floats;
            break;
        case 15:
            if (self->roster_slot_0x888 == -1) {
                roster = cmd->roster;
                if (roster != NULL) {
                    ((void (*)(_ENEMY_WORK*, s32, s32*, s32, s32))em_roster_slot_effect_set)(
                        self, roster->base + (u8)em_roster_record_result_get((EmPopRec*)self), &self->roster_slot_0x888,
                        roster->kind, 0);
                } else {
                    em_roster_default_effect_set(self);
                }
            }
            break;
        case 16:
            if (self->roster_slot_0x888 != -1) {
                em_roster_record_release(self->roster_slot_0x888);
                self->roster_slot_0x888 = -1;
            }
            break;
        case 17:
            self->seq_cond_0x8E4 = cmd->cond;
            break;
        case 18:
            self->seq_value_0x904 = cmd->count;
            break;
        case 21:
            self->seq_mot_0x8E0 = NULL;
            return 0;
        case 22:
            em_height_set(self, *cmd->floats);
            break;
        case 23:
            self->pos.y = self->pos.y + self->field_0x1ac;
            em_height_set(self, 0.0f);
            break;
        case 24:
            if (self->action_0x1E5 == 11) {
                self->carve_count_0x89E++;
            }
            em_drop_item_spawn(self, &self->pos, cmd->count);
            break;
        case 25:
            floats = cmd->floats;
            self->seq_move_frame_0x908 = floats[0];
            self->seq_move_gate_0x90C = floats[1];
            break;
        case 26:
            em_anger_add(self, (s16)cmd->value);
            break;
        case 27:
            self->field_0x8D3 = 0;
            break;
        case 28:
            em_stamina_add(self, (s16)-(s16)cmd->value);
            break;
        case 29:
            self->seq_flags_0x8D1 |= 0x10;
            em_guard_set(self);
            break;
        case 30:
            self->seq_flags_0x8D1 &= ~0x10;
            break;
        case 31:
            move = cmd->move;
            em_move_vec2_clr(self);
            copyVec3(&self->v_0x310, &move->velocity);
            copyVec3(&self->v_0x31C, &move->accel);
            mode = move->mode[0] & ~0xC0;
            if (mode <= 2 || (u32)(mode - 4) <= 2 || mode == 3 || mode == 7) {
                jitter_x = (f32)(s32)(self->bits_0x1EC & 0xF000) / 65536.0f;
                jitter_y = (f32)(s32)((self->bits_0x1EC & 0xF00) << 4) / 65536.0f;
                jitter_z = (f32)(s32)((self->bits_0x1EC & 0xF0) << 8) / 65536.0f;
            }
            if (!(move->mode[0] & 0x80)) {
                jitter_x = jitter_x * 2.0f - 1.0f;
                jitter_y = jitter_y * 2.0f - 1.0f;
                jitter_z = jitter_z * 2.0f - 1.0f;
            }
            if (mode <= 3) {
                self->v_0x310.x = self->v_0x310.x + move->limit.x * jitter_x;
                self->v_0x310.y = self->v_0x310.y + move->limit.y * jitter_y;
                self->v_0x310.z = self->v_0x310.z + move->limit.z * jitter_z;
            } else if ((s32)mode >= 4 && (s32)mode <= 7) {
                self->v_0x31C.x = self->v_0x31C.x + move->limit.x * jitter_x;
                self->v_0x31C.y = self->v_0x31C.y + move->limit.y * jitter_y;
                self->v_0x31C.z = self->v_0x31C.z + move->limit.z * jitter_z;
            }
            if (move->mode[0] & 0x40) {
                vec3_scale_in_place(&self->v_0x310, get_em_chg_scale(self));
                vec3_scale_in_place(&self->v_0x31C, get_em_chg_scale(self));
            }
            self->seq_rot_0x8F8 = self->pos_0x1BC.y;
            break;
        case 32:
            eft_rot_vec_copy(&self->pos_0x1BC, cmd->rot);
            break;
        case 33:
            self->seq_flags_0x8D1 |= 0x20;
            em_busy_timer_reset(self);
            break;
        case 34:
            self->seq_flags_0x8D1 |= 0x40;
            em_motion_timer_arm(self);
            break;
        case 35:
            self->seq_hook_0x910 = cmd->hook;
            break;
        case 0xFE:
            state = cmd->state;
            if (state != NULL) {
                em_state_set(self, state->state, state->sub);
                return 2;
            }
            switch (self->state_sub) {
            case 0xB6:
            case 0xBA:
            case 0xBB:
            case 0xBC:
                em_state_set(self, 10, 0xB7);
                break;
            case 0xB7:
                em_state_set(self, 10, 0xB8);
                break;
            case 0xBF:
                em_state_set(self, 10, 0xC1);
                break;
            case 0xC0:
                em_state_set(self, 10, 0xC2);
                break;
            case 0x7B:
            case 0x81:
            case 0x94:
            case 0xF8:
                em_state_set(self, 10, 0x9F);
                break;
            case 0x8A:
                em_state_set(self, 10, 0xA1);
                break;
            case 0x7C:
            case 0x82:
            case 0x95:
                em_state_set(self, 10, 0x8D);
                break;
            case 0x9B:
                em_state_set(self, 10, 0xA6);
                break;
            case 0x7D:
            case 0x7E:
            case 0x7F:
            case 0x83:
            case 0x96:
                em_state_set(self, 10, 0x8E);
                break;
            case 0x8B:
                em_state_set(self, 10, 0x8F);
                break;
            case 0x9C:
                em_state_set(self, 10, 0x91);
                break;
            case 0x8C:
                em_state_set(self, 10, 0x90);
                break;
            case 0x9D:
                em_state_set(self, 10, 0x92);
                break;
            case 0xE4:
                em_state_set(self, 10, 0xE5);
                break;
            case 0xE7:
            case 0xE8:
            case 0xE9:
                em_state_set(self, 10, 0xE4);
                break;
            case 0xEA:
                em_state_set(self, 10, 0xEB);
                break;
            case 0xED:
            case 0xEE:
            case 0xEF:
                em_state_set(self, 10, 0xEA);
                break;
            default:
                return 1;
            }
            return 2;
        case 0xFD:
            switch (self->state_sub) {
            case 0x7A:
                em_act_set_unless_act11(self, 10, 0x7B);
                break;
            case 0x80:
                em_act_set_unless_act11(self, 10, 0x81);
                break;
            case 0x93:
                em_act_set_unless_act11(self, 10, 0x94);
                break;
            case 0x9A:
                em_act_set_unless_act11(self, 10, 0x9B);
                break;
            case 0x89:
                em_act_set_unless_act11(self, 10, 0x8A);
                break;
            case 0xB9:
                em_act_set_unless_act11(self, 10, 0xBA);
                break;
            case 0xE6:
                em_act_set_unless_act11(self, 10, 0xE7);
                break;
            case 0xEC:
                em_act_set_unless_act11(self, 10, 0xED);
                break;
            default:
                return 1;
            }
            return 2;
        }
        cmd = &seq[self->seq_index_0x8D0];
    }
    return 1;
}

/* 0x80124150 - moves the enemy by the current motion record's move kind and re-applies the per-frame calls the
 * script armed; returns the axes whose speed changed sign (1/2/4) and the height crossings (8/0x10). */
extern "C" u8 em_seq_move_update(_ENEMY_WORK* self)
{
    u8 flags = 0;
    nw4r::math::VEC3 before;
    nw4r::math::VEC3 unused;
    f32 height;
    _CP_VECTOR rot;
    EmSeqMove* move;

    VEC3_ctor(&before);
    VEC3_ctor(&unused);
    if (self->seq_stage_0x8D2 == 0 && self->seq_mot_0x8E0 != NULL) {
        if (self->seq_turn_0x8F0 > 0.0f) {
            em_turn_in_window(self, self->seq_turn_rate_0x8EC, self->seq_turn_0x8F0, self->seq_turn_angle_0x8F4);
        }
        switch (self->seq_mot_0x8E0->move_kind) {
        case 1:
        case 2:
        case 3:
        case 7:
        case 8:
        case 5:
            if (self->seq_move_gate_0x90C > 0.0f &&
                em_frame_check(self, 4, self->seq_move_frame_0x908, self->seq_move_gate_0x90C) == 0) {
                break;
            }
            copyVec3(&before, &self->v_0x310);
            if (self->seq_mot_0x8E0->move_kind == 1) {
                CancelFade(self);
            } else if ((u8)(self->seq_mot_0x8E0->move_kind + 249) <= 1) {
                height = self->field_0x1ac;
                em_height_move_step(self);
                if ((height < 0.0f && self->field_0x1ac >= 0.0f) || (height >= 0.0f && self->field_0x1ac < 0.0f)) {
                    flags |= 8;
                }
                if (self->field_0x1ac >= 0.0f) {
                    flags |= 0x10;
                }
            } else {
                rot.x = self->pos_0x1BC.x;
                rot.y = self->seq_rot_0x8F8;
                rot.z = self->pos_0x1BC.z;
                em_move_offset_step(self, &rot);
            }
            if ((before.x < 0.0f && self->v_0x310.x >= 0.0f) || (before.x >= 0.0f && self->v_0x310.x < 0.0f)) {
                flags |= 1;
            }
            if ((before.y < 0.0f && self->v_0x310.y >= 0.0f) || (before.y >= 0.0f && self->v_0x310.y < 0.0f)) {
                flags |= 2;
            }
            if ((before.z < 0.0f && self->v_0x310.z >= 0.0f) || (before.z >= 0.0f && self->v_0x310.z < 0.0f)) {
                flags |= 4;
            }
            if (self->seq_mot_0x8E0->move_kind == 5 || self->seq_mot_0x8E0->move_kind == 8) {
                move = self->seq_mot_0x8E0->move;
                switch (move->mode[0]) {
                case 1:
                    if (self->v_0x310.x > move->limit.x) {
                        self->v_0x310.x = move->limit.x;
                    }
                    break;
                case 2:
                    if (self->v_0x310.x < move->limit.x) {
                        self->v_0x310.x = move->limit.x;
                    }
                    break;
                }
                switch (move->mode[1]) {
                case 1:
                    if (self->v_0x310.y > move->limit.y) {
                        self->v_0x310.y = move->limit.y;
                    }
                    break;
                case 2:
                    if (self->v_0x310.y < move->limit.y) {
                        self->v_0x310.y = move->limit.y;
                    }
                    break;
                }
                switch (move->mode[2]) {
                case 1:
                    if (self->v_0x310.z > move->limit.z) {
                        self->v_0x310.z = move->limit.z;
                    }
                    break;
                case 2:
                    if (self->v_0x310.z < move->limit.z) {
                        self->v_0x310.z = move->limit.z;
                    }
                    break;
                }
            }
            break;
        case 4:
            if (!(self->seq_move_gate_0x90C > 0.0f) ||
                em_frame_check(self, 4, self->seq_move_frame_0x908, self->seq_move_gate_0x90C) != 0) {
                em_move_offset_apply(self);
            }
            break;
        case 6:
            if (self->seq_mot_0x8E0->move != NULL) {
                em_height_add(self, em_key_curve_eval(self, self->seq_mot_0x8E0->move));
            }
            break;
        }
    }
    if (self->seq_flags_0x8D1 & 2) {
        em_flag_0x116_set(self);
    }
    if (self->seq_flags_0x8D1 & 4) {
        em_flag_0x115_set(self);
    }
    if (self->seq_flags_0x8D1 & 0x10) {
        em_guard_set(self);
    }
    if (self->seq_flags_0x8D1 & 8) {
        em_frame_flag_set(self);
    }
    if (self->seq_flags_0x8D1 & 0x20) {
        em_busy_timer_reset(self);
    }
    if (self->seq_flags_0x8D1 & 0x40) {
        em_motion_timer_arm(self);
    }
    return flags;
}

/* 0x801245A0 - whether the current step's wait is over: the `wait` mask picks the tests (motion end, a frame,
 * landing heights, ground, the timer, the script value against three counters, the move flags). */
extern "C" u32 em_seq_wait_ck(_ENEMY_WORK* self, EmSeqCmd* seq, u8 moved, u16 wait)
{
    EmSeqMotion* motion;

    if ((wait & 1) && em_mot_end_ck(self) == 1) {
        return 1;
    }
    if ((wait & 8) && em_frame_check(self, 1, self->seq_frame_0x8E8, 0.0f) == 1) {
        if (self->seq_flags_0x8D1 & 1) {
            motion = self->seq_mot_0x8E0;
            em_mot_set_blend(self, motion->motion, motion->blend, motion->frame, motion->loop);
            if (self->seq_speed_0x900 > 0.0f) {
                em_mot_speed_set(self, self->seq_speed_0x900);
            }
        } else {
            return 1;
        }
    }
    if ((wait & 0x10) && self->pos.y <= self->field_0x210) {
        self->seq_index_0x8D0 = em_seq_label_find(seq, self->seq_index_0x8D0, 0xFF);
        em_move_mode_set(self, 2);
        return 1;
    }
    if ((wait & 0x20) && self->pos.y <= self->field_0x210) {
        return 1;
    }
    if ((wait & 0x40) && self->pos.y >= self->field_0x210) {
        return 1;
    }
    if ((wait & 2) && ((u32 (*)(_ENEMY_WORK*, f32))(u32 (*)(_ENEMY_WORK*))em_ground_ck)(self, self->seq_ground_0x8FC) == 1) {
        self->pos.y = self->field_0x20C;
        return 1;
    }
    if ((wait & 4) && --self->seq_timer_0x20 <= 0) {
        return 1;
    }
    if ((wait & 0x100) && self->field_0x950 <= self->seq_value_0x904) {
        return 1;
    }
    if ((wait & 0x200) && self->field_0x952 <= self->seq_value_0x904) {
        return 1;
    }
    if ((wait & 0x80) && (moved & 2)) {
        return 1;
    }
    if ((wait & 0x400) && self->field_0x916 <= self->seq_value_0x904) {
        return 1;
    }
    if ((wait & 0x800) && (moved & 8)) {
        return 1;
    }
    if ((wait & 0x1000) && (moved & 0x10)) {
        return 1;
    }
    return 0;
}

/* 0x801248E4 - whether a dead enemy's carcass is held (no despawn): the move work, a quest gate or the arena
 * flags keep it. */
extern "C" u32 em_seq_death_hold_ck(_ENEMY_WORK* self)
{
    if (move_work_state_ck() == 1 || !(self->field_0x1C8 & 1) || (u32)em_spawn_rec_live_ck(self) == 1 ||
        quest_flag_4000000_ck(0) == 1 ||
        (quest_flag_2000000_or_80000000_ck(0) == 1 && quest_field32C_or_4000000_ck() == 1) ||
        ((s8)self->field_0x00D != 0 && quest_field32C_or_4000000_ck() == 1)) {
        return 1;
    }
    return 0;
}

/* 0x8012498C - whether the current step is done: the motion step waits on its mask, the death stages on their
 * timers and the carcass records. */
extern "C" u32 em_seq_step_ck(_ENEMY_WORK* self, EmSeqCmd* cmd, u8 moved)
{
    u32 done = 0;
    u8 released;
    s32 i;
    EmCarrySlot* carry;
    u16 wait;
    EmSeqMotion* motion;

    switch (self->seq_stage_0x8D2) {
    default:
        if (self->action_0x1E5 == 11) {
            self->seq_timer_0x914++;
        }
        if (self->alpha_0x1D4 <= 0.0f) {
            done = 1;
        } else {
            if (cmd != NULL && cmd->code == 21) {
                wait = cmd->value;
            } else {
                wait = self->seq_mot_0x8E0->flags;
            }
            if (em_seq_wait_ck(self, cmd, moved, wait) == 1) {
                done = 1;
            }
        }
        break;
    case 1:
        if (em_seq_death_hold_ck(self) == 0 && em_act_kind6_ck(self->action_0x1E5, self->state_sub) == 0) {
            if (self->roster_slot_0x888 != -1 && em_roster_record_slot_id_get(self->roster_slot_0x888) <= 0) {
                em_roster_record_release(self->roster_slot_0x888);
                self->roster_slot_0x888 = -1;
            }
        } else {
            if (!(self->field_0x1C8 & 1) && self->roster_slot_0x888 != -1) {
                released = em_roster_record_slot_id_get(self->roster_slot_0x888) <= 0;
                if (released != 0) {
                    carry = self->carry_0x488;
                    for (i = 0; i < 2; i++, carry++) {
                        if (carry->roster_0x80 != -1) {
                            if (em_roster_record_slot_id_get(carry->roster_0x80) <= 0) {
                                released = 1;
                            } else {
                                released = 0;
                                break;
                            }
                        }
                    }
                }
                if (released == 1) {
                    done = 1;
                }
            }
            if (--self->seq_timer_0x20 <= 0) {
                done = 1;
            }
        }
        break;
    case 2:
        if (self->alpha_0x1D4 <= 0.0f) {
            self->visible_0x001 = 0;
        }
        if (--self->seq_timer_0x20 <= 0) {
            done = 1;
        }
        break;
    case 3:
        if (--self->seq_timer_0x20 <= 0) {
            em_motion_param_set(self, 0.0f, 0);
            done = 1;
        }
        if (self->alpha_0x1D4 <= 0.0f) {
            self->visible_0x001 = 0;
        }
        break;
    }
    if (done == 1) {
        if (self->seq_stage_0x8D2 == 0) {
            self->seq_flags_0x8D1 &= 0x99;
            self->seq_turn_rate_0x8EC = 0.0f;
            self->seq_turn_0x8F0 = 0.0f;
            self->seq_turn_angle_0x8F4 = 0;
        }
        return 1;
    }
    if ((self->seq_flags_0x8D1 & 1) && em_mot_end_ck(self) == 1) {
        motion = self->seq_mot_0x8E0;
        em_mot_set_blend(self, motion->motion, 0, motion->frame, motion->loop);
        if (self->seq_speed_0x900 > 0.0f) {
            em_mot_speed_set(self, self->seq_speed_0x900);
        }
    }
    return 0;
}

/* 0x80124C5C - runs an action script (`em_se_tbl_play`'s table): the first call arms the script and runs it to its
 * first motion, later calls move the enemy, step the script and, at its end, finish the action (the death action's
 * carcass stages first). */
extern "C" void em_seq_run(_ENEMY_WORK* self, EmSeqCmd* seq, u8 drop)
{
    nw4r::math::VEC3 pos;
    u32 done;
    u32 index;
    u32 breath;
    u32 joint;

    VEC3_ctor(&pos);
    if (self->state_0x05 == 0) {
        self->state_0x05++;
        if (drop == 1) {
            em_joint_drop_start(self, self->joint_0x204);
        } else {
            em_move_mode_set(self, drop);
        }
        em_seq_stamina_cost(self);
        self->seq_index_0x8D0 = 0;
        self->seq_0x8D4 = seq;
        self->field_0x8D8 = 0;
        self->field_0x8DC = 0;
        self->seq_mot_0x8E0 = NULL;
        self->seq_cond_0x8E4 = NULL;
        self->seq_frame_0x8E8 = 0.0f;
        self->seq_flags_0x8D1 = 0;
        self->seq_turn_rate_0x8EC = 0.0f;
        self->seq_turn_0x8F0 = 0.0f;
        self->seq_turn_angle_0x8F4 = 0;
        self->seq_move_frame_0x908 = 0.0f;
        self->seq_move_gate_0x90C = 0.0f;
        self->seq_rot_0x8F8 = self->pos_0x1BC.y;
        self->seq_stage_0x8D2 = 0;
        self->seq_ground_0x8FC = em_fall_height_get(self);
        self->seq_speed_0x900 = -1.0f;
        self->seq_value_0x904 = 0.0f;
        self->seq_timer_0x914 = 0;
        self->seq_hook_0x910 = NULL;
        em_seq_exec(self, self->seq_0x8D4, 1);
        em_user_data_reenter(self);
    } else {
        done = em_seq_step_ck(self, self->seq_0x8D4, em_seq_move_update(self));
        if (done == 0 && self->seq_cond_0x8E4 != NULL && em_seq_cond_ck(self, self->seq_cond_0x8E4) == 1) {
            done = 1;
        }
        if (done == 1) {
            self->seq_value_0x904 = 0.0f;
            if (self->seq_cond_0x8E4 != NULL) {
                self->seq_cond_0x8E4 = NULL;
            }
            switch (em_seq_exec(self, self->seq_0x8D4, 0)) {
            case 0:
                self->state_0x05++;
                break;
            case 2:
                break;
            default:
                em_move_vec2_clr(self);
                if (self->action_0x1E5 == 11) {
                    switch (self->seq_stage_0x8D2) {
                    case 0:
                        self->seq_stage_0x8D2++;
                        if (self->alpha_0x1D4 <= 0.0f && !(self->field_0x1C8 & 0x800)) {
                            self->seq_timer_0x20 = 0;
                            if (self->roster_slot_0x888 != -1) {
                                em_roster_record_release(self->roster_slot_0x888);
                                self->roster_slot_0x888 = -1;
                            }
                        } else {
                            if (em_seq_death_hold_ck(self) == 0 &&
                                em_act_kind6_ck(self->action_0x1E5, self->state_sub) == 1) {
                                self->seq_timer_0x20 = 0x834;
                            } else {
                                self->seq_timer_0x20 = get_enemy_data(self)->frames_bias_0x5C;
                            }
                            if (em_captured_ck(self) == 0) {
                                if (self->roster_slot_0x888 == -1) {
                                    em_roster_default_effect_set(self);
                                }
                            } else if (self->roster_slot_0x888 != -1) {
                                em_roster_record_release(self->roster_slot_0x888);
                                self->roster_slot_0x888 = -1;
                            }
                        }
                        self->timer_0x18 = em_frames_get(self);
                        break;
                    case 1:
                        self->seq_stage_0x8D2++;
                        if (self->alpha_0x1D4 <= 0.0f) {
                            self->seq_timer_0x20 = 0;
                        } else {
                            self->seq_timer_0x20 = 100;
                        }
                        em_roster_records_release(self);
                        break;
                    case 2:
                        self->seq_stage_0x8D2++;
                        if (self->alpha_0x1D4 <= 0.0f) {
                            self->seq_timer_0x20 = 0;
                        } else {
                            self->seq_timer_0x20 = 60;
                            ((void (*)(_ENEMY_WORK*, s16))em_motion_param_set)(self, 60);
                        }
                        break;
                    case 3:
                        if ((u32)em_spawn_rec_live_ck(self) == 1) {
                            self->state_0x1E7 = 0;
                            if (self->field_0x1C8 & 1) {
                                em_act_request(self, 12, 0xFF);
                            } else {
                                em_act_request_now(self, 12, 0xFF);
                            }
                        } else {
                            self->field_0x017 = 3;
                        }
                        break;
                    }
                } else {
                    switch (self->state_0x1E2) {
                    default:
                        em_action_finish(self);
                        break;
                    case 1:
                        switch (self->field_0x1E3) {
                        case 1:
                            em_action_finish_dive(self);
                            break;
                        case 2:
                            em_action_finish_sub2(self);
                            break;
                        default:
                            em_action_finish_fall(self);
                            break;
                        }
                        break;
                    case 2:
                        em_action_finish_swim(self);
                        break;
                    case 3:
                        em_action_finish_walk(self);
                        break;
                    case 4:
                        em_action_finish_stand(self);
                        break;
                    }
                }
                break;
            }
        }
    }
    if (self->state_0x1E2 == 1) {
        ((void (*)(_ENEMY_WORK*, f32))UpdateValue)(self, self->seq_ground_0x8FC);
    }
    if (em_act_trap_ck(self) == 1 || em_act_paralysis_ck(self) == 1 || em_act_sleep_ck(self) == 1) {
        em_fx_flag2_set(self);
    }
    if (self->seq_hook_0x910 != NULL) {
        self->seq_hook_0x910(self);
    }
    if (em_area_ck(self) == 1 && (em_act_ck(self, 10, 0xBA) != 0 || em_act_ck(self, 11, 0x1C) != 0)) {
        index = ((system_w.field_0x0c & 0x7F) + self->group * 31) & 0x7F;
        switch (index) {
        case 0:
            breath = 0;
            break;
        case 0x40:
            breath = 1;
            break;
        default:
            breath = 0xFF;
            break;
        }
        if (breath != 0xFF) {
            joint = *((EmAilmentFxView*)get_enemy_data(self)->ailment_fx_0x9C)->sleep_joint;
            if (joint == (u32)-1) {
                joint = 2;
            }
            get_joint_wpos_em(self, joint, &pos);
            ((void (*)(_se_w*, VEC3*, u32))se_em_snore_req)(self->se_handle_0xB14, &pos, breath);
        }
    }
}
