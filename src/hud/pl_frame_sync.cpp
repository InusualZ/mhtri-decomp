/* hud/pl_frame_sync.cpp - the player act frame handlers and the character-state network sync.
 * RANGE. .text 0x8033041C..0x80338808 (125 functions, all written), extab 0x8001662C..0x80016994, extabindex
 *   0x800357A8..0x80035CC4, .data 0x805E0510..0x805E27D4, .sdata 0x80792D28..0x80792D38, .sdata2
 *   0x8079B210..0x8079B2B8.  Three families in address order: the frame effect's state bodies and the
 *   paired-single matrix helpers (0x8033041C..0x80331104); the player act handlers and their frame helpers
 *   (`pl_mode8_*`, `pl_frame_hit_set`, `pl_act_attr_stage_set`, `pl_act_hit_*`, `pl_act_handler_00..29`,
 *   0x80331104..0x80334568); the network sync (`Pl_net_*`, `em_net_*`, `emc_net_*`, `eft_net_*`, from
 *   `Pl_act_pair_enter` on).  A sender packs a player work, an enemy work, the enemy-control work or an effect
 *   slot into one wire message (`hud/net_char_sync.h`) for `broadcastSessionCommand`; the receiver applies it.
 * SEAM. 0x8033041C..0x80331104 is the tail of `enemy/em_act_step_tail.cpp`'s TU, not this one: that unit creates
 *   the frame effect and dispatches the four `eft_frame_state_*` bodies, `.sdata2` 30.0f is pooled twice
 *   (0x8079B218 by `eft_frame_state_step`, 0x8079B258 by `pl_act_lunge_slide`) - one value, two pools, two TUs -
 *   and `.data` pads 4 bytes at 0x805E0634 between the frame effect's tables and the hit tables.  Its data is
 *   .data 0x805E0510..0x805E0638, .sdata 0x80792D28..0x80792D38 and .sdata2 0x8079B210..0x8079B238.
 * FLAGS. `cflags_hud` (cflags_main + `-opt nopeephole`); `eft_frame_state_step` under `#pragma pool_data off`
 *   (each key table its own lis/addi) and `#pragma fp_contract off` (fmuls+fadds).
 * NAMES. GUESS (from the bodies and their callers): the stem; `pl_mode8_free_ck`, `pl_mode8_alive_ck`,
 *   `pl_gauge_level_get`, `pl_charge_flash_ck`, `pl_frame_hit_set`, `pl_act_attr_stage_set`, `pl_act_hit_quake`,
 *   `pl_act_hit_react`, `pl_act_lunge_slide`, `pl_act_lunge_slide_xyz`, the thirty `pl_act_handler_NN` (the
 *   dispatcher's act handlers in address order), `mtx34_to_mtx33` (it inverts the 3x3 part),
 *   `mtx34_inverse_transpose`, `mtx34_inverse_affine`, `mtx34_inverse_epsilon`, `eft_frame_state_*`, the data
 *   rows `pl_frame_hit_shape_a/b/c`, `pl_frame_hit_shapes`, `pl_frame_hit_table`, `pl_act_steps_NN`,
 *   `pl_act_steps_table_enter`, `pl_flinch_steps_kind8(_guard)`, `pl_steps_kind8(_guard)`,
 *   `pl_steps_kind8_slot3(_guard)`; the registry callees `pl_sub_gauge_add`, `pl_hit_level_max_get`,
 *   `pl_impact_se_req`, `eft019_shot_set`.  The network family's roles (`send_state`/`_alt` = kinds 1/9,
 *   `send_pos` 2, `send_hit` 3/5, `send_extra` 6, `send_item` 7/8; the enemy kinds `state`, `target`, `flags`,
 *   `act`, `count`, `bind`, `release`; `emc_*`, `eft_*`).
 * RESIDUALS.
 *   - the SEAM above: one object cannot pool 30.0f twice, so `.sdata2` is 160 of 168 bytes and the unit cannot
 *     flip until the range is re-drawn; `.data` is byte-identical (8900 B), `.sdata` 11 of 16 bytes (tail pad).
 *   - `eft_frame_state_step` (+8 B): the colour-step switch lowers as compare pairs, retail as `subi`+`cmplwi`
 *     range tests on 17..19 and 1..2; `pos.y`'s load is scheduled before the 0.5f multiply.
 *   - `pl_act_handler_11`/`_21` (+24 B): retail shares one `pl_act_enter` block between the gauge-empty exit and
 *     the finisher exit (a jump back into the earlier branch); with no `goto` the two exits are two calls.
 *   - `pl_act_handler_10`/`_29`: the shell table pointer loads before `mr r3`/`li r4` (retail after).
 *   - `pl_act_attr_stage_set`: retail narrows the merged motion with `clrlwi` before `Pl_chr_set_attr_default`.
 *   - `em_net_send_count` (r0/r3 swap), `em_net_parts_pack`/`_unpack` (the part index re-derived into r0),
 *     `em_net_recv_target` (three temporaries' stack slots).
 *   - the step tables carry dtk's guessed relocations on words like 0x80190016 (they are data, the linked bytes
 *     agree); `EftFrameStateWork` is `enemy/em_act_step_tail.cpp`'s `EftFrameWork` block (merged by the seam).
 * SHAPES. `&get_worldworld_pos(...)` as `copyVec3`'s source; a `switch` with `default:` first (the `& 0x50`
 *   selections, `emc_net_recv_status`); `(u32)pl_part_flag2_ck(...) == 1` (retail `cmplwi`); the impact act's
 *   `(u32)(step - 2) > 1` range test; `(i * 240) / 2` for the frame records' key frames; local-declaration
 *   order for `pl_frame_hit_set`/`pl_act_hit_react`'s registers; the act tables are each handler's own local
 *   statics and the step tables are defined after `Pl_act_frame_dispatch` (the `.data` emission order).
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
#include "ef/eft007_set.h"
#include "hud/mtx34_inverse_affine.h"
#include "ef/eft019_shot_set.h"
#include "sound/pl_impact_se_req.h"
#include "Pl/pl_hit_level_max_get.h"
#include "Pl/pl_sub_gauge_add.h"
#include "Pl/pl_hit_result_best_get.h"
#include "Pl/fn_8027D684.h"
#include "camera/camera_main.h"
#include "stage/shell_set_func_ptr.h"
#include "hud/eft_frame_state_init.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "ef/eft_stage_spawn_flag.h"
#include "ef/GameMode_ck.h"
#include "ai/ai_npc.h"
#include "lobby/lb_server_sel_trans.h"

/* One act variant's two attr stages: stage 0 (`motion` 0xFFFF skips it) and stage 1, which also arms the
 * act's step table.  size: 0x10 */
typedef struct PlActAttrRow {
    /* +0x00 */ u16 motion;        /* stage 0's motion, 0xFFFF for none */
    /* +0x02 */ s16 attr_a;
    /* +0x04 */ s16 attr_b;
    /* +0x06 */ u16 turn;          /* added to the actor's yaw (+0x058) and its copy (+0x0A8) */
    /* +0x08 */ u16 motion_next;   /* stage 1's motion */
    /* +0x0A */ s16 attr_a_next;
    /* +0x0C */ s16 attr_b_next;
    /* +0x0E */ s16 hit_next;      /* the hit row `pl_frame_hit_set` arms, 0 for none */
} PlActAttrRow;

/* One variant of the guard act's attr: its motion and the attr the motion enters with.  size: 0x4 */
typedef struct PlActGuardRow {
    /* +0x00 */ u16 motion;
    /* +0x02 */ s16 attr;
} PlActGuardRow;

/* One record of the frame effect's pool block: the flash model, its effect, the position and the colour and
 * alpha key frames.  size: 0x34 */
typedef struct EftFrameStateRec {
    /* +0x00 */ MHchar* model;
    /* +0x04 */ nw4r::ef::Effect* effect;
    /* +0x08 */ VEC3 pos;
    /* +0x14 */ s32 color_frame;   /* the colour key frame, 0..240 */
    /* +0x18 */ s32 alpha_frame;   /* the alpha key frame, 0..60 */
    /* +0x1C */ s32 spin;          /* copied into the flash model's rotation x */
    /* +0x20 */ s32 seed;          /* a `ran_suu` roll, copied into the flash model's rotation y */
    /* +0x24 */ u8 unused_0x24[0x4];
    /* +0x28 */ _GXColor color;    /* the flash model's material colour; the alpha follows the alpha keys */
    /* +0x2C */ f32 scale;
    /* +0x30 */ u8 flash;          /* 1 while the flash model is shown */
    /* +0x31 */ u8 pad_0x31[0x3];
} EftFrameStateRec;

/* The frame effect's pool block (`_EFT::work_0x38`): the record count and the records.  size: 0x4 + 0x34 per
 * record (the count is the block's own; an approximation of one record is given). */
typedef struct EftFrameStateWork {
    /* +0x00 */ u8 count;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ EftFrameStateRec recs[1];
} EftFrameStateWork;

/* The joint holder `_PLW::physics_0x13C` points at: the player's `MHchar` sits at +0x04.  size: 0x144 - lower
 * bound. */
typedef struct EftFramePhysics {
    /* +0x000 */ u8 unused_0x000[0x4];
    /* +0x004 */ MHchar chr_0x04;
} EftFramePhysics;

extern "C" {

/* 0x8033041C (0x1DC): sets the frame effect up: places it on the source's joint and creates one flash model and
 * effect per record, then runs the first step. */
void eft_frame_state_init(_EFT* self) {
    EftFrameStateWork* work;
    EftFrameStateRec* rec;
    f32 scale;
    s32 i;
    u8 opaque;
    u8 off;

    work = (EftFrameStateWork*)self->work_0x38;
    rec = work->recs;
    self->state_0x05 += 1;
    scale = rec->scale;
    switch (self->field_0x07) {
    case 0: {
        _PLW* plw = (_PLW*)self->source_0x30;
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(0, &self->pos_0x18);
        break;
    }
    case 2: {
        _AINPC_W* npc = (_AINPC_W*)self->source_0x30;
        if (eft_res_spawn_gate_ck(self, 2) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        npc->model.get_joint_wpos(0, &self->pos_0x18);
        break;
    }
    }
    i = 0;
    opaque = 255;
    off = 0;
    for (; i < work->count; i++, rec++) {
        if (res_eft_model_create(rec->model, 62, 4) == 0) {
            eft_frame_state_nop(self);
            return;
        }
        rec->effect = res_eft_create(504, 9, 0);
        if (rec->effect == NULL) {
            eft_frame_state_nop(self);
            return;
        }
        rec->color_frame = (i * 240) / 2;
        rec->alpha_frame = (i * 60) / 2;
        rec->scale = scale;
        copyVec3(&rec->pos, &self->pos_0x18);
        rec->color.r = opaque;
        rec->color.g = opaque;
        rec->color.b = opaque;
        rec->color.a = opaque;
        rec->seed = ran_suu(0);
        rec->flash = off;
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 2;
    eft_frame_state_step(self);
}

#pragma push
#pragma pool_data off
#pragma fp_contract off
/* 0x803305F8 (0x808): follows the source's joint each frame, steps every record's colour and alpha keys, and
 * re-spawns the flash model on the records that flash. */
void eft_frame_state_step(_EFT* self) {
    static u8 color_keys_heat[0x30] = {
        0x00, 0xFF, 0x00, 0x00, 0x18, 0xFF, 0x69, 0x00, 0x30, 0xFF, 0x00, 0x00, 0x48, 0xFF, 0x69, 0x00,
        0x60, 0xFF, 0x00, 0x00, 0x78, 0xFF, 0x69, 0x00, 0x90, 0xFF, 0x00, 0x00, 0xA8, 0xFF, 0x69, 0x00,
        0xC0, 0xFF, 0x00, 0x00, 0xD8, 0xFF, 0x69, 0x00, 0xF0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    };
    static u8 color_keys_cold[0x30] = {
        0x00, 0xFF, 0x00, 0x00, 0x18, 0x0A, 0x7F, 0x6F, 0x30, 0xFF, 0x00, 0x00, 0x48, 0x0A, 0x7F, 0x6F,
        0x60, 0xFF, 0x00, 0x00, 0x78, 0x0A, 0x7F, 0x6F, 0x90, 0xFF, 0x00, 0x00, 0xA8, 0x0A, 0x7F, 0x6F,
        0xC0, 0xFF, 0x00, 0x00, 0xD8, 0x0A, 0x7F, 0x6F, 0xF0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    };
    static u8 color_keys_mix[0x30] = {
        0x00, 0xFF, 0x69, 0x00, 0x18, 0x0A, 0x7F, 0x6F, 0x30, 0xFF, 0x69, 0x00, 0x48, 0x0A, 0x7F, 0x6F,
        0x60, 0xFF, 0x69, 0x00, 0x78, 0x0A, 0x7F, 0x6F, 0x90, 0xFF, 0x69, 0x00, 0xA8, 0x0A, 0x7F, 0x6F,
        0xC0, 0xFF, 0x69, 0x00, 0xD8, 0x0A, 0x7F, 0x6F, 0xF0, 0xFF, 0x69, 0x00, 0xFF, 0xFF, 0x69, 0x00,
    };
    static u8 color_keys_cycle[0x38] = {
        0x00, 0xFF, 0x00, 0x00, 0x14, 0xFF, 0x69, 0x00, 0x28, 0x0A, 0x7F, 0x6F, 0x3C, 0xFF, 0x00, 0x00,
        0x50, 0xFF, 0x69, 0x00, 0x64, 0x0A, 0x7F, 0x6F, 0x78, 0xFF, 0x00, 0x00, 0x8C, 0xFF, 0x69, 0x00,
        0xA0, 0x0A, 0x7F, 0x6F, 0xB4, 0xFF, 0x00, 0x00, 0xC8, 0xFF, 0x69, 0x00, 0xDC, 0x0A, 0x7F, 0x6F,
        0xF0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    };
    static u8 alpha_keys[0xC] = {
        0x00, 0x19, 0x0A, 0xFF, 0x1E, 0xFF, 0x3C, 0x00, 0xFF, 0x00, 0x00, 0x00,
    };
    static u8 color_red[3] = { 0xFF, 0x00, 0x00 };
    static u8 color_orange[3] = { 0xFF, 0x69, 0x00 };
    static u8 color_teal[3] = { 0x0A, 0x7F, 0x6F };
    VEC3 pos;
    VEC3 unused;
    VEC3 joint_a;
    VEC3 joint_b;
    VEC3 spawn_pos;
    EftFrameStateWork* work;
    EftFrameStateRec* rec;
    s32 mode;
    s32 i;
    _GXColor color;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused);
    VEC3_ctor(&joint_a);
    VEC3_ctor(&joint_b);
    VEC3_ctor(&spawn_pos);
    work = (EftFrameStateWork*)self->work_0x38;
    rec = work->recs;
    if (eft_res_spawn_gate_ck(self, self->field_0x07) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05 += 1;
        return;
    }
    if (GameMode_ck() == 2) {
        return;
    }
    mode = 1;
    switch (self->field_0x07) {
    case 0: {
        _PLW* plw = (_PLW*)self->source_0x30;
        self->flag_0x01 = plw->field_0x001;
        self->area_0x44 = plw->area_0x16;
        if (self->demo_flag_0x08 == 0xFF) {
            return;
        }
        if (self->area_0x44 != get_now_areano()) {
            return;
        }
        if (event_demo_ck() == 1) {
            return;
        }
        if (Pl_master_ck(plw) == 1) {
            mode = eft_stage_spawn_flag(plw) | 1;
        }
        ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(0, &pos);
        copyVec3(&self->pos_0x18, &pos);
        ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(14, &pos);
        copyVec3(&spawn_pos, &pos);
        if (plw->kind_0x09 == 3) {
            pos.y = self->pos_0x18.y - 120.0f;
        } else if (plw->kind_0x09 == 2) {
            ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(14, &joint_a);
            pos.y = joint_a.y - 120.0f;
        } else {
            ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(20, &joint_a);
            ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(17, &joint_b);
            if (pos.y > 30.0f + 0.5f * (joint_a.y + joint_b.y)) {
                ((EftFramePhysics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(14, &joint_a);
                pos.y = joint_a.y - 120.0f;
            } else {
                pos.y = self->pos_0x18.y - 20.0f;
            }
        }
        switch (plw->field_0x3D8 & 0x50) {
        default:
            self->field_0x06 = 0;
            break;
        case 0x10:
            self->field_0x06 = 1;
            break;
        case 0x40:
            self->field_0x06 = 2;
            break;
        case 0x50:
            self->field_0x06 = 3;
            break;
        }
        if ((plw->field_0x3DC & 0xF8000) != 0) {
            self->field_0x06 |= 0x10;
        }
        break;
    }
    case 2: {
        _AINPC_W* npc = (_AINPC_W*)self->source_0x30;
        self->flag_0x01 = npc->in_area;
        self->area_0x44 = npc->area;
        if (self->demo_flag_0x08 == 0xFF) {
            return;
        }
        if (self->area_0x44 != get_now_areano()) {
            return;
        }
        if (event_demo_ck() == 1) {
            return;
        }
        if (npc->motion == 1 && (npc->motion_step == 83 || npc->motion_step == 85)) {
            return;
        }
        npc->model.get_joint_wpos(0, &pos);
        copyVec3(&self->pos_0x18, &pos);
        npc->model.get_joint_wpos(3, &pos);
        copyVec3(&spawn_pos, &pos);
        switch (npc->variant) {
        case 3:
            pos.y = self->pos_0x18.y - 60.0f;
            break;
        case 2:
            npc->model.get_joint_wpos(3, &joint_a);
            pos.y = joint_a.y - 60.0f;
            break;
        default:
            npc->model.get_joint_wpos(20, &joint_a);
            npc->model.get_joint_wpos(18, &joint_b);
            if (pos.y > 30.0f + 0.5f * (joint_a.y + joint_b.y)) {
                npc->model.get_joint_wpos(3, &joint_a);
                pos.y = joint_a.y - 60.0f;
            } else {
                pos.y = self->pos_0x18.y - 20.0f;
            }
            break;
        }
        switch (npc->behaviour_flags & 0x50) {
        default:
            self->field_0x06 = 0;
            break;
        case 0x10:
            self->field_0x06 = 1;
            break;
        case 0x40:
            self->field_0x06 = 2;
            break;
        case 0x50:
            self->field_0x06 = 3;
            break;
        }
        break;
    }
    }
    for (i = 0; i < work->count; i++, rec++) {
        switch (self->field_0x06) {
        case 0:
            copyVec3(&rec->pos, &pos);
            break;
        case 1:
        case 2:
        case 16:
            rec->color_frame += 2;
            if (rec->flash == 1) {
                rec->alpha_frame += 4;
            } else {
                rec->alpha_frame += 2;
            }
            break;
        case 3:
        case 17:
        case 18:
        case 19:
            rec->color_frame += 4;
            if (rec->flash == 1) {
                rec->alpha_frame += 6;
            } else {
                rec->alpha_frame += 3;
            }
            break;
        }
        if (rec->color_frame > 240) {
            rec->color_frame = 0;
        }
        copyVec3(&rec->pos, &pos);
        if (rec->alpha_frame >= 60) {
            if (++self->field_0x10 >= 2) {
                self->field_0x10 = 0;
                rec->flash = 1;
            } else {
                rec->flash = 0;
            }
            rec->seed = ran_suu(0);
            rec->alpha_frame = 0;
        }
        SetRootMtxTrans(rec->effect, &rec->pos);
        switch (self->field_0x06) {
        case 0:
            continue;
        case 1:
            color.r = color_red[0];
            color.g = color_red[1];
            color.b = color_red[2];
            color.a = 255;
            break;
        case 2:
            color.r = color_orange[0];
            color.g = color_orange[1];
            color.b = color_orange[2];
            color.a = 255;
            break;
        case 3:
            eftGetKeyRGB(color_keys_heat, rec->color_frame, &color.r, &color.g, &color.b);
            color.a = 255;
            break;
        case 16:
            color.r = color_teal[0];
            color.g = color_teal[1];
            color.b = color_teal[2];
            color.a = 255;
            break;
        case 17:
            eftGetKeyRGB(color_keys_cold, rec->color_frame, &color.r, &color.g, &color.b);
            color.a = 255;
            break;
        case 18:
            eftGetKeyRGB(color_keys_mix, rec->color_frame, &color.r, &color.g, &color.b);
            color.a = 255;
            break;
        case 19:
            eftGetKeyRGB(color_keys_cycle, rec->color_frame, &color.r, &color.g, &color.b);
            color.a = 255;
            break;
        }
        change_paramscale_eff(rec->effect, rec->scale);
        effect_move(rec->effect);
        change_color_eff(rec->effect, &rec->pos, color);
        eft_res_models_spawn(self, (void**)&rec->effect, mode, 1, &spawn_pos);
        if (rec->flash == 1) {
            rec->color.a = eftGetKeyAlpha(alpha_keys, rec->alpha_frame);
            rec->model->setMatColor(0, GX_COLOR0A0, rec->color, false);
            rec->model->scale_0x1C.x = rec->scale;
            rec->model->scale_0x1C.y = rec->scale * (f32)rec->alpha_frame / 60.0f;
            rec->model->scale_0x1C.z = rec->scale;
            copyVec3(&rec->model->pos_0x04, &rec->pos);
            rec->model->field_0x28 = rec->spin;
            rec->model->field_0x2C = rec->seed;
            rec->model->field_0x30 = 0;
            rec->model->move(0);
            mode = (mode & ~1) | 2;
            eft_res_models_spawn(self, (void**)rec, mode, 1, NULL);
        }
    }
}

#pragma pop

/* 0x80330E00 (0x10): ends the frame effect (the next state). */
void eft_frame_state_end(_EFT* self) {
    self->state_0x05 += 1;
}

/* 0x80330E10 (0x4): releases the frame effect's slot. */
void eft_frame_state_nop(_EFT* self) {
    eft_res_slot_release(self);
}

} /* extern "C" */

/* One hit volume of the frame hit table: two shape ids, the point count, the points' coordinates and the end
 * marker.  size: 0x50 */
typedef struct PlFrameHitShape {
    /* +0x00 */ u16 shape_a;
    /* +0x02 */ u16 shape_b;
    /* +0x04 */ u16 point_count;
    /* +0x06 */ u16 pad_0x06;
    /* +0x08 */ f32 coords[8];
    /* +0x28 */ u16 end_marker;     /* 0xFFFF */
    /* +0x2A */ u16 pad_0x2A;
    /* +0x2C */ u8 unused_0x2C[0x24];
} PlFrameHitShape;

/* The act step tables `Pl_act_set_step_table` runs: rows of 0x16 bytes ended by a 0xFF row, defined after the
 * act dispatcher (their emission place). */
extern u16 pl_act_steps_01[0x42];
extern u16 pl_act_steps_02[0x42];
extern u16 pl_act_steps_03[0x58];
extern u16 pl_act_steps_04[0x58];
extern u16 pl_act_steps_05[0x79];
extern u16 pl_act_steps_06[0x42];
extern u16 pl_act_steps_07[0x2C];
extern u16 pl_act_steps_08[0x42];
extern u16 pl_act_steps_15[0x37];
extern u16 pl_act_steps_09[0x16];
extern u16 pl_act_steps_10[0x2C];
extern u16 pl_act_steps_11[0xB];
extern u16 pl_act_steps_13a[0x16];
extern u16 pl_act_steps_13b[0x16];
extern u16 pl_act_steps_14[0x79];
extern u16 pl_act_steps_16[0x58];
extern u16 pl_act_steps_17[0x58];
extern u16 pl_act_steps_18[0x58];
extern u16 pl_act_steps_19[0x6E];
extern u16 pl_act_steps_20[0x42];
extern u16 pl_act_steps_21[0xB];
extern u16 pl_act_steps_23[0x6E];
extern u16 pl_act_steps_24[0x8F];
extern u16 pl_act_steps_25[0x8F];
extern u16 pl_act_steps_26a[0x16];
extern u16 pl_act_steps_26b[0x16];
extern u16 pl_act_steps_27[0x58];
extern u16 pl_act_steps_table_enter[0x4D];
extern u16 pl_act_steps_28[0x16];
extern u16 pl_act_steps_29[0x42];
extern u16 pl_flinch_steps_kind8[0x16];
extern u16 pl_flinch_steps_kind8_guard[0x16];
extern u16 pl_steps_kind8[0xE7];
extern u16 pl_steps_kind8_guard[0x108];
extern u16 pl_steps_kind8_slot3[0xBB];
extern u16 pl_steps_kind8_slot3_guard[0xDC];

/* 0x805E0638 / 0x805E0688 / 0x805E06D8: the frame hit table's three hit volumes. */
PlFrameHitShape pl_frame_hit_shape_a = { 7, 7, 1, 0, { 0.0f, 52.0f, 0.0f, 0.0f, 60.0f, 0.0f, 0.0f, 260.0f }, 0xFFFF, 0 };
PlFrameHitShape pl_frame_hit_shape_b = { 7, 7, 1, 0, { 0.0f, 52.0f, 0.0f, 0.0f, 50.0f, 0.0f, 0.0f, 220.0f }, 0xFFFF, 0 };
PlFrameHitShape pl_frame_hit_shape_c = { 11, 11, 3, 0, { 0.0f, 52.0f, -0.5f, -38.0f, -27.0f, 0.0f, 0.0f, 200.0f }, 0xFFFF, 0 };

/* 0x805E0728: the hit volumes `pl_hit_attack_build` resolves the frame hit rows' volume ids through. */
PlFrameHitShape* pl_frame_hit_shapes[3] = { &pl_frame_hit_shape_a, &pl_frame_hit_shape_b, &pl_frame_hit_shape_c };

/* 0x805E0734: the frame hit rows `pl_frame_hit_set` arms (the attack records `pl_hit_attack_build` reads). */
u8 pl_frame_hit_table[0x32C] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x0E, 0x00, 0x36,
    0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x06, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x0C, 0x00, 0x19, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xEC, 0x04, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C,
    0x00, 0x16, 0x00, 0x26, 0x03, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x0A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00,
    0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x14, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x14,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x0C, 0x00, 0x22,
    0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0xEC, 0x02, 0x04, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x0C, 0x00, 0x1E, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E,
    0x00, 0x06, 0x00, 0x1E, 0x02, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x24, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x30, 0x00, 0x28, 0x02, 0x00,
    0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x5E, 0x00, 0x08, 0x00, 0x28, 0x02, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x0F, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x25, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x0E, 0x00, 0x16, 0x01, 0x00, 0x00, 0x28,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x38, 0x00, 0x0E, 0x00, 0x36, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x04, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x0C, 0x00, 0x19,
    0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xEC, 0x04, 0x04, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x16, 0x00, 0x26, 0x03, 0x00, 0x00, 0x28, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28,
    0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00,
    0x00, 0x14, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
    0x00, 0x14, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x14, 0x01, 0x00, 0x00, 0x14,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x26, 0x00, 0x0C, 0x00, 0x22, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01,
    0x01, 0x00, 0xEC, 0x02, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x0C, 0x00, 0x1E,
    0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x06, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x00, 0x06, 0x00, 0x1E, 0x02, 0x00, 0x00, 0x1E, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x02, 0x24, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x0F, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x25, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x0E, 0x00, 0x16, 0x01, 0x00,
    0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x06, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x28, 0x00, 0x0C, 0x00, 0x19, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0xEC, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x0C,
    0x00, 0x19, 0x01, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xEC, 0x00,
    0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

extern "C" {

/* 0x8079B230: the determinant magnitude below which the matrix helpers treat a matrix as singular. */
static const f32 mtx34_inverse_epsilon[2] = { 1.0000000359391298e-36f, 0.0f };

/* 0x80330E14 (0xE4): writes the inverse of `src`'s 3x3 part into `dst`; returns 0 (the part copied) when it is singular. */
asm u32 mtx34_to_mtx33(nw4r::math::MTX33* dst, const MTX34* src) {
    nofralloc
    psq_l    f0, 0(r4), 1, 0
    psq_l    f1, 4(r4), 0, 0
    psq_l    f2, 16(r4), 1, 0
    ps_merge10 f6, f1, f0
    psq_l    f3, 20(r4), 0, 0
    psq_l    f4, 32(r4), 1, 0
    ps_merge10 f7, f3, f2
    psq_l    f5, 36(r4), 0, 0
    ps_mul   f11, f3, f6
    ps_merge10 f8, f5, f4
    ps_mul   f13, f5, f7
    ps_msub  f11, f1, f7, f11
    ps_mul   f12, f1, f8
    ps_msub  f13, f3, f8, f13
    lis      r12, mtx34_inverse_epsilon@ha
    ps_msub  f12, f5, f6, f12
    ps_mul   f7, f0, f13
    addi     r12, r12, mtx34_inverse_epsilon@l
    ps_mul   f10, f3, f4
    ps_madd  f7, f2, f12, f7
    psq_l    f6, 0(r12), 1, 0
    ps_mul   f9, f0, f5
    ps_madd  f7, f4, f11, f7
    ps_msub  f10, f2, f5, f10
    ps_msub  f9, f1, f4, f9
    ps_abs   f8, f7
    ps_cmpo0 cr0, f8, f6
    bge      invertible
    psq_st   f0, 0(r3), 1, 0
    psq_st   f2, 12(r3), 1, 0
    psq_st   f4, 24(r3), 1, 0
    psq_st   f1, 4(r3), 0, 0
    psq_st   f3, 16(r3), 0, 0
    psq_st   f5, 28(r3), 0, 0
    li       r3, 0
    blr     
invertible:
    fres     f5, f7
    ps_mul   f8, f1, f2
    ps_sub   f6, f6, f6
    ps_add   f1, f5, f5
    ps_msub  f8, f0, f3, f8
    ps_mul   f2, f7, f5
    ps_nmsub f0, f5, f2, f1
    ps_muls0 f13, f13, f0
    ps_muls0 f12, f12, f0
    psq_st   f13, 0(r3), 0, 0
    ps_muls0 f11, f11, f0
    psq_st   f12, 12(r3), 0, 0
    ps_muls0 f10, f10, f0
    psq_st   f11, 24(r3), 0, 0
    ps_muls0 f9, f9, f0
    psq_st   f10, 8(r3), 1, 0
    ps_muls0 f8, f8, f0
    psq_st   f9, 20(r3), 1, 0
    psq_st   f8, 32(r3), 1, 0
    li       r3, 1
    blr     
}

/* 0x80330EF8 (0x108): writes the inverse transpose of `src`'s 3x3 part into `dst` with a zero translation; returns 0 when it is singular. */
asm u32 mtx34_inverse_transpose(MTX34* dst, const MTX34* src) {
    nofralloc
    psq_l    f0, 0(r4), 1, 0
    psq_l    f1, 4(r4), 0, 0
    psq_l    f2, 16(r4), 1, 0
    ps_merge10 f6, f1, f0
    psq_l    f3, 20(r4), 0, 0
    psq_l    f4, 32(r4), 1, 0
    ps_merge10 f7, f3, f2
    psq_l    f5, 36(r4), 0, 0
    ps_mul   f11, f3, f6
    ps_merge10 f8, f5, f4
    ps_mul   f13, f5, f7
    ps_msub  f11, f1, f7, f11
    ps_mul   f12, f1, f8
    ps_msub  f13, f3, f8, f13
    lis      r12, mtx34_inverse_epsilon@ha
    ps_msub  f12, f5, f6, f12
    ps_mul   f7, f0, f13
    addi     r12, r12, mtx34_inverse_epsilon@l
    ps_mul   f10, f3, f4
    ps_madd  f7, f2, f12, f7
    psq_l    f6, 0(r12), 1, 0
    ps_mul   f9, f0, f5
    ps_madd  f7, f4, f11, f7
    ps_msub  f10, f2, f5, f10
    ps_msub  f9, f1, f4, f9
    ps_abs   f8, f7
    ps_cmpo0 cr0, f8, f6
    bge      invertible
    cmplw    r3, r4
    beq      singular
    psq_st   f0, 0(r3), 1, 0
    psq_st   f2, 16(r3), 1, 0
    psq_st   f4, 32(r3), 1, 0
    ps_sub   f6, f6, f6
    psq_st   f1, 4(r3), 0, 0
    psq_st   f3, 20(r3), 0, 0
    psq_st   f5, 36(r3), 0, 0
    psq_st   f6, 12(r3), 1, 0
    psq_st   f6, 28(r3), 1, 0
    psq_st   f6, 44(r3), 1, 0
singular:
    li       r3, 0
    blr     
invertible:
    fres     f5, f7
    ps_mul   f8, f1, f2
    ps_sub   f6, f6, f6
    ps_add   f1, f5, f5
    ps_msub  f8, f0, f3, f8
    psq_st   f6, 12(r3), 1, 0
    ps_mul   f2, f7, f5
    psq_st   f6, 28(r3), 1, 0
    psq_st   f6, 44(r3), 1, 0
    ps_nmsub f0, f5, f2, f1
    ps_muls0 f13, f13, f0
    ps_muls0 f12, f12, f0
    psq_st   f13, 0(r3), 0, 0
    ps_muls0 f11, f11, f0
    psq_st   f12, 16(r3), 0, 0
    ps_muls0 f10, f10, f0
    psq_st   f11, 32(r3), 0, 0
    ps_muls0 f9, f9, f0
    psq_st   f10, 8(r3), 1, 0
    ps_muls0 f8, f8, f0
    psq_st   f9, 24(r3), 1, 0
    psq_st   f8, 40(r3), 1, 0
    li       r3, 1
    blr     
}

/* 0x80331000 (0x104): writes the inverse of the affine matrix `src` into `dst`; returns 0 when it is singular. */
asm u32 mtx34_inverse_affine(MTX34* dst, const MTX34* src) {
    nofralloc
    psq_l    f0, 0(r4), 1, 0
    psq_l    f1, 4(r4), 0, 0
    psq_l    f2, 16(r4), 1, 0
    ps_merge10 f6, f1, f0
    psq_l    f3, 20(r4), 0, 0
    psq_l    f4, 32(r4), 1, 0
    ps_merge10 f7, f3, f2
    psq_l    f5, 36(r4), 0, 0
    ps_mul   f11, f3, f6
    ps_merge10 f8, f5, f4
    ps_mul   f13, f5, f7
    ps_msub  f11, f1, f7, f11
    ps_mul   f12, f1, f8
    ps_msub  f13, f3, f8, f13
    lis      r12, mtx34_inverse_epsilon@ha
    ps_msub  f12, f5, f6, f12
    ps_mul   f7, f0, f13
    addi     r12, r12, mtx34_inverse_epsilon@l
    ps_mul   f10, f3, f4
    ps_madd  f7, f2, f12, f7
    psq_l    f6, 0(r12), 1, 0
    ps_mul   f9, f0, f5
    ps_madd  f7, f4, f11, f7
    ps_msub  f10, f2, f5, f10
    ps_msub  f9, f1, f4, f9
    ps_abs   f8, f7
    ps_cmpo0 cr0, f8, f6
    bge      invertible
    li       r3, 0
    blr     
invertible:
    fres     f6, f7
    ps_mul   f8, f1, f2
    ps_add   f5, f6, f6
    ps_mul   f4, f7, f6
    ps_msub  f8, f0, f3, f8
    ps_nmsub f0, f6, f4, f5
    lfs      f1, 12(r4)
    ps_muls0 f13, f13, f0
    lfs      f2, 28(r4)
    ps_muls0 f12, f12, f0
    lfs      f3, 44(r4)
    ps_muls0 f11, f11, f0
    ps_merge00 f5, f13, f12
    ps_merge11 f4, f13, f12
    ps_mul   f6, f13, f1
    psq_st   f5, 0(r3), 0, 0
    psq_st   f4, 16(r3), 0, 0
    ps_muls0 f10, f10, f0
    ps_muls0 f9, f9, f0
    ps_madd  f6, f12, f2, f6
    psq_st   f10, 32(r3), 1, 0
    ps_muls0 f8, f8, f0
    ps_nmadd f6, f11, f3, f6
    psq_st   f9, 36(r3), 1, 0
    ps_mul   f7, f10, f1
    ps_merge00 f5, f11, f6
    psq_st   f8, 40(r3), 1, 0
    ps_madd  f7, f9, f2, f7
    ps_merge11 f4, f11, f6
    psq_st   f5, 8(r3), 0, 0
    ps_nmadd f7, f8, f3, f7
    psq_st   f4, 24(r3), 0, 0
    psq_st   f7, 44(r3), 1, 0
    li       r3, 1
    blr     
}

/* 0x80331104 (0x28): returns 1 while the actor is in mode 8 and not in the guard variant. */
u32 pl_mode8_free_ck(_PLW* plw) {
    if (plw->field_0x002 == 8 && plw->field_0x5C6 == 0) {
        return 1;
    }
    return 0;
}

/* 0x8033112C (0x74): returns 1 while the actor is alive in mode 8 and free to act. */
u32 pl_mode8_alive_ck(_PLW* plw) {
    if (plw->field_0x00A == 8 || plw->health <= 0) {
        return 0;
    }
    if (plw->field_0x002 != 8) {
        return 0;
    }
    if (plw->field_0x018 == 0 || pl_mode8_free_ck(plw) == 1) {
        return 1;
    }
    return 0;
}

/* 0x803311A0 (0x70): returns the gauge level 0-4 from the +0x36C gauge (4 when empty and not alive). */
u8 pl_gauge_level_get(_PLW* plw) {
    s32 level = 4;
    s16 gauge = plw->field_0x36C;
    if (gauge >= 44) {
        level = 0;
    } else if (gauge >= 42) {
        level = 1;
    } else if (gauge >= 1) {
        level = 2;
    } else if (pl_mode8_alive_ck(plw) == 0) {
        level = 3;
    }
    return level;
}

/* 0x80331210 (0x28): returns the colour index of the actor's blade record. */
u8 pl_frame_eft_color_get(_PLW* plw) {
    return equip_blade_rec_get(&plw->equipB)->phial_type;
}

/* 0x80331238 (0xF4): arms hit row `hit` (negative: mirrored) of the frame hit table for the actor. */
void pl_frame_hit_set(_PLW* plw, s16 hit, u8 solo) {
    s32 flags;
    u16 index;

    if (hit < 0) {
        index = -hit;
        flags = 0x10;
    } else {
        index = hit;
        flags = 0;
    }
    if (solo != 0) {
        pl_hit_attack_build(plw, plw->hit_0x484, pl_frame_hit_table, index, (s32*)pl_frame_hit_shapes, flags | 4);
    } else if (pl_mode8_free_ck(plw) == 1) {
        pl_hit_attack_build(plw, plw->hit_0x484, pl_frame_hit_table, index, (s32*)pl_frame_hit_shapes, flags | 7);
    } else {
        pl_hit_attack_build(plw, plw->hit_0x484, pl_frame_hit_table, index, (s32*)pl_frame_hit_shapes, flags | 0xF);
    }
    plw->field_0x565 = 0;
}

/* 0x8033132C (0xA4): returns 1 while the mode-8 actor's charge flash shows. */
u32 pl_charge_flash_ck(_PLW* plw) {
    if (plw->field_0x002 != 8) {
        return 0;
    }
    if (pl_mode8_free_ck(plw) == 0 && plw->field_0x018 == 1) {
        return 0;
    }
    if (plw->field_0x00A == 4 && (u32)(plw->act_no - 70) <= 1) {
        return 1;
    }
    if ((u32)(pl_gauge_level_get(plw) - 2) <= 2) {
        return 1;
    }
    return 0;
}

/* 0x803313D0 (0xE0): applies one attr stage of an act variant's row; the first stage that applies arms the
 * act's step table. */
void pl_act_attr_stage_set(_PLW* plw, u8 stage, PlActAttrRow* row, u16* steps) {
    u16 motion;
    u16 turn;
    s16 attr_a;
    s16 attr_b;
    s16 hit;

    if (stage == 1 || (motion = row->motion) == 0xFFFF) {
        motion = row->motion_next;
        attr_a = row->attr_a_next;
        attr_b = row->attr_b_next;
        hit = row->hit_next;
        turn = 0;
        Pl_act_set_step_table(plw, (u32)steps, 0);
        if (stage == 0) {
            plw->act_step_0x05 += 1;
        }
    } else {
        attr_a = row->attr_a;
        attr_b = row->attr_b;
        hit = 0;
        turn = row->turn;
    }
    Pl_chr_set_attr_default(plw, motion, attr_a, attr_b);
    if (turn != 0) {
        plw->field_0x058 += turn;
        plw->field_0x0A8 = plw->field_0x058;
    }
    if (hit != 0) {
        pl_frame_hit_set(plw, hit, 0);
    }
}

/* 0x803314B0 (0xA8): requests the hit's camera quake of `kind` (or the strong one once the hit level
 * reaches 3). */
void pl_act_hit_quake(_PLW* plw, u8 kind, u8 extra) {
    u8 quake;

    if (pl_hit_level_max_get(plw) >= 3 || kind == 2) {
        if (kind == 0 || kind == 2) {
            quake = 0;
        } else {
            quake = 1;
        }
    } else {
        if (kind == 0) {
            return;
        }
        quake = 7;
    }
    pl_act_hold_notify(plw, 2);
    camera_pl_quake_req(plw, quake | extra);
}

/* 0x80331558 (0x310): reacts to the act's best hit result once (recoil, the quake) and requests the
 * frame-`frame` quake. */
void pl_act_hit_react(_PLW* plw, f32 frame) {
    u8 hold;
    u8 extra;
    u8 quake;

    if (plw->field_0x565 == 0) {
        quake = 255;
        hold = 0;
        extra = 0;
        switch (plw->act_no) {
        case 9:
        case 22:
        case 53:
        case 54:
        case 72:
        case 86:
        case 87:
        case 88:
            quake = 2;
            hold = 1;
            break;
        case 24:
        case 55:
            quake = 2;
            hold = 1;
            break;
        case 10:
        case 56:
        case 83:
        case 84:
            extra = 0;
            hold = 1;
            break;
        default:
            if (plw->field_0x5C6 == 1 && plw->field_0x56B >= 2) {
                hold = 1;
            }
            extra = 0;
            break;
        }
        switch (pl_hit_result_best_get(plw)) {
        case 2:
            plw->field_0x565 = 1;
            if (quake == 255) {
                quake = 0;
            }
            break;
        case 3:
        case 4:
            plw->field_0x565 = 1;
            if (quake == 255) {
                quake = 1;
            }
            break;
        case 0:
        case 5:
            plw->field_0x565 = 1;
            if (quake == 255) {
                quake = 1;
            }
            if (hold != 1) {
                if (plw->kind_0x09 == 3) {
                    if (plw->field_0x5C6 == 1) {
                        pl_act_enter(plw, 4, 50, 0x4080);
                    } else {
                        pl_act_enter(plw, 4, 49, 0x4080);
                    }
                } else if (plw->field_0x5C6 == 1) {
                    pl_act_enter(plw, 4, 18, 0x4000);
                } else {
                    pl_act_enter(plw, 4, 17, 0x4000);
                }
            }
            break;
        case 1:
            plw->field_0x565 = 1;
            if (hold != 1) {
                s32 attr_frame = pl_rig_get_motion_frame(plw) - 10.0f;
                Pl_chr_set_attr_default(plw, Get_motion_no(plw), 10, attr_frame);
                plw->field_0x354 = 0.7f;
            }
            break;
        }
        if (pl_hit_result_best_get(plw) != 255 && quake != 255) {
            pl_act_hit_quake(plw, quake, extra);
        }
    }
    if (plw->field_0x566 == 0 && plw->field_0x565 == 0 && frame > 0.0f && Pl_frame_check(plw, 0, frame, 0.0f) == 1) {
        plw->field_0x566 = 1;
        camera_pl_quake_req(plw, 0);
    }
}

/* 0x80331868 (0x15C): the guard act: enters the variant's motion and guard attr, then the actor's idle slot. */
void pl_act_handler_00(_PLW* plw, s32 variant) {
    static PlActGuardRow rows[4] = {
            { 0x475, 0 },
            { 0x474, 0 },
            { 0x4A7, 4 },
            { 0x4A6, 0 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x018 = 1;
        pl_act_clear_flag5bb(plw);
        switch (variant) {
        case 0:
            plw->field_0x5C6 = 0;
            Pl_act_set_motion(plw, 0, 0, 0);
            break;
        case 1:
            plw->field_0x5C6 = 1;
            Pl_act_set_motion(plw, 0, 0, 0);
            break;
        case 2:
            plw->field_0x5C6 = 0;
            Pl_act_set_motion(plw, 3, 0, 0);
            break;
        case 3:
            plw->field_0x5C6 = 1;
            Pl_act_set_motion(plw, 3, 0, 0);
            break;
        }
        Pl_chr_set_attr_default(plw, rows[variant].motion, rows[variant].attr, 0);
        if (plw->field_0x5C6 == 0) {
            plw->guard_timer_0x36E = 150;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, plw->kind_0x09, 4, 0);
        }
        break;
    }
}

/* 0x803319C4 (0x120): a three-stage attack act on motion slot 0; variant 1 also steps the act timer. */
void pl_act_handler_01(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x44D, 4, 0, 1 },
        { 0x459, 2, 0, 0, 0x44D, 0, 0x36, 1 },
        { 0x455, 4, 0, 0, 0x44D, 0, 0x34, 1 },
        { 0x457, 4, 0, 0, 0x44D, 0, 0x34, 1 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_01);
        if (variant == 1) {
            pl_act_timer_step(plw);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_01);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80331AE4 (0x110): a two-variant attack act on motion slot 0. */
void pl_act_handler_02(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[2] = {
        { 0xFFFF, 0, 0, 0, 0x44F, 4, 0, 3 },
        { 0xFFFF, 0, 0, 0, 0x44F, 4, 4, 3 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_02);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_02);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}


/* 0x80331BF4 (0x110): a one-variant attack act on motion slot 0. */
void pl_act_handler_03(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x450, 4, 0, 4 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_03);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_03);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}


/* 0x80331D04 (0xF0): slides the actor forward on the three lunge motions' frame windows. */
void pl_act_lunge_slide(_PLW* plw, s32 variant) {
    VEC3 step;
    f32 start;
    f32 end;

    VEC3_ctor(&step);
    step.x = 0.0f;
    step.y = 0.0f;
    step.z = 0.0f;
    if ((u32)(variant - 5) <= 2) {
        step.z = 28.0f;
    }
    switch (Get_motion_no(plw)) {
    case 1120:
        start = 28.0f;
        end = 36.0f;
        break;
    case 1130:
        start = 18.0f;
        end = 26.0f;
        break;
    case 1132:
        start = 30.0f;
        end = 38.0f;
        break;
    default:
        return;
    }
    if (Pl_frame_check(plw, 3, start, end) == 1) {
        rotVecY(&step, plw->field_0x058);
        plw->field_0x03C += step.x;
        plw->field_0x044 += step.z;
    }
}

/* 0x80331DF4 (0x158): the lunge attack act on motion slot 0: charges, then slides on its frames. */
void pl_act_handler_04(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[9] = {
        { 0xFFFF, 0, 0, 0, 0x460, 4, 0, 9 },
        { 0x476, 6, 0, 0, 0x460, 0, 0x26, 9 },
        { 0x468, 2, 0, 0, 0x460, 0, 0x20, 9 },
        { 0x46A, 2, 0, 0, 0x460, 0, 0x26, 9 },
        { 0x46C, 4, 0, 0, 0x460, 0, 0x26, 9 },
        { 0x46C, 4, 0, 0, 0x460, 0, 0x26, 9 },
        { 0xFFFF, 0, 0, 0, 0x460, 4, 0, 9 },
        { 0x46A, 2, 0, 0, 0x460, 0, 0x26, 9 },
        { 0x476, 6, 0, 0, 0x460, 0, 0x26, -9 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        plw->field_0x5C6 = 1;
        plw->field_0x018 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_set_motion_gate(plw, 70);
        pl_act_clear_flag5bb(plw);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_04);
        if (variant == 2) {
            pl_act_timer_step(plw);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_04);
        }
        pl_act_lunge_slide(plw, variant);
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
            pl_act_lunge_slide(plw, variant);
        }
        break;
    }
}

/* 0x80331F4C (0x124): a charged attack act on motion slot 0. */
void pl_act_handler_05(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[3] = {
        { 0xFFFF, 0, 0, 0, 0x461, 4, 0, 0xA },
        { 0x466, 2, 0, 0, 0x461, 0, 0x2C, 0xA },
        { 0xFFFF, 0, 0, 0, 0x461, 0xE, 0x2C, 0xA },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        plw->field_0x5C6 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_05);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_05);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80332070 (0x128): an unguarded attack act on motion slot 0. */
void pl_act_handler_06(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[5] = {
        { 0xFFFF, 0, 0, 0, 0x44E, 4, 0, 2 },
        { 0x458, 2, 0, 0, 0x44E, 0, 0x26, -29 },
        { 0x477, 6, 0, 0, 0x44E, 0, 0x26, 2 },
        { 0x456, 4, 6, 0, 0x44E, 0, 0x26, 2 },
        { 0x456, 4, 0, 0, 0x44E, 0, 0x26, 2 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        plw->field_0x018 = 1;
        plw->field_0x5C6 = 0;
        pl_act_set_motion_gate(plw, 70);
        pl_act_clear_flag5bb(plw);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_06);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_06);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80332198 (0x28C): the combo-finisher act on motion slot 0: counts the hits and enters the finisher. */
void pl_act_handler_07(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x451, 4, 0, 0 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        plw->field_0x007 = 0;
        Pl_act_set_motion(plw, 0, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_07);
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_07);
        }
        break;
    case 2:
        if (Pl_master_ck(plw) != 0 && Pl_frame_check(plw, 0, 90.0f, 0.0f) == 1) {
            if (Pl_master_ck(plw) == 1 && plw->field_0x006 >= 5 && plw->field_0x378 > 0) {
                pl_act_enter(plw, 4, 11, 0);
            } else {
                pl_act_enter(plw, 4, 12, 0);
            }
            break;
        }
        if (Pl_frame_check(plw, 0, 56.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        } else if (Pl_frame_check(plw, 0, 88.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        }
        if (plw->field_0x007 != 0) {
            plw->field_0x007 -= 1;
        }
        if (Pl_master_ck(plw) == 1 && (u32)pl_part_flag2_ck(plw, 12) == 1 && plw->field_0x007 == 0) {
            plw->field_0x006 += 1;
            plw->field_0x007 = 3;
        }
        pl_act_hit_react(plw, -1.0f);
        if (Pl_frame_check(plw, 0, 34.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 5, 0);
        } else if (Pl_frame_check(plw, 0, 72.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 6, 0);
        }
        break;
    }
}

/* 0x80332424 (0x1FC): the impact act on motion slot 0: spawns the two impact effects on its frames. */
void pl_act_handler_08(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x462, 6, 0, 0xB },
        { 0xFFFF, 0, 0, 0, 0x462, 6, 0, 0xB },
        { 0xFFFF, 0, 0, 0, 0x462, 6, 0, 0xB },
        { 0xFFFF, 0, 0, 0, 0x462, -26, 0x18, 0xB },
    };
    VEC3 pos;
    s32 step;

    VEC3_ctor(&pos);
    pl_act_set_motion_gate(plw, 2);
    step = plw->act_step_0x05;
    if ((u32)(step - 2) > 1) {
        switch (step) {
        case 0:
            plw->act_step_0x05 += 1;
            Pl_act_set_motion(plw, 0, 1, 0);
            plw->field_0x5C6 = 1;
            plw->field_0x310 = 0;
            pl_act_add_charge(plw, -11);
            pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_08);
            if (variant == 3) {
                pl_impact_pos_get(plw, &pos);
                pl_impact_se_req(plw, &pos);
            }
            break;
        case 1:
            if (Pl_motion_end_ck(plw) == 1) {
                plw->act_step_0x05 += 1;
                pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_08);
            }
            break;
        }
    } else {
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        if (Pl_frame_check(plw, 0, 42.0f, 0.0f) == 1) {
            setVector3(&pos, -23.0f, 6.0f, 21.0f);
            eft007_set(plw, 4, 0, 11, &pos, 1.0f);
        }
        if (Pl_frame_check(plw, 0, 78.0f, 0.0f) == 1) {
            setVector3(&pos, -33.0f, 12.0f, 18.0f);
            eft007_set(plw, 6, 0, 11, &pos, 1.0f);
        }
    }
}

/* 0x80332620 (0x1EC): the charge act on motion slot 0: counts the charge and enters its release. */
void pl_act_handler_09(_PLW* plw) {
    pl_act_set_motion_gate(plw, 2);
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        plw->field_0x007 = 0;
        Pl_act_set_motion(plw, 0, 1, 0);
        plw->field_0x5C6 = 1;
        Pl_chr_setX(plw, 0x463, 10, 0);
        Pl_act_set_step_table(plw, (u32)pl_act_steps_09, 0);
        plw->field_0x28 = 30;
        plw->charge_gauge_0x310 += 1;
        plw->count_0x034 = 0;
        pl_frame_hit_set(plw, 14, 0);
        pl_act_add_charge(plw, -3);
        break;
    case 1:
        if (Pl_master_ck(plw) == 1) {
            if (plw->field_0x007 != 0) {
                plw->field_0x007 -= 1;
            }
            if ((u32)pl_part_flag2_ck(plw, 7) == 1 && plw->field_0x007 == 0) {
                plw->field_0x006 += 1;
                plw->field_0x007 = 3;
            }
            if (--plw->field_0x28 <= 0) {
                if (plw->field_0x006 >= 4 && pl_gauge_level_get(plw) <= 2) {
                    if (plw->charge_gauge_0x310 >= 2) {
                        pl_act_enter(plw, 4, 10, 0);
                    } else {
                        pl_act_enter(plw, 4, 24, 0);
                    }
                } else {
                    pl_act_enter(plw, 4, 25, 0);
                }
            }
        }
        if (++plw->count_0x034 >= 10) {
            plw->count_0x034 = 0;
            pl_frame_hit_set(plw, 14, 0);
        }
        break;
    }
}

/* 0x8033280C (0x1D0): the shot act on motion slot 0: spawns the shot at its frame. */
void pl_act_handler_10(_PLW* plw, s32 variant) {
    VEC3 pos;
    VEC3 base;
    VEC3 ofs;
    VEC3 sum;

    VEC3_ctor(&pos);
    VEC3_ctor(&base);
    VEC3_ctor(&ofs);
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        Pl_chr_set_attr_default(plw, 0x465, 2, 0);
        Pl_act_set_step_table(plw, (u32)pl_act_steps_10, 0);
        pl_act_set_motion_gate(plw, 70);
        plw->field_0x5C6 = 0;
        pl_act_add_charge(plw, -20);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else if (Pl_frame_check(plw, 0, 12.0f, 0.0f) == 1) {
            copyVec3(&base, &plw->vec_0x03C);
            ofs.x = -30.0f;
            ofs.y = 180.0f;
            ofs.z = 155.0f;
            rotVecY(&ofs, plw->field_0x058);
            addVec3(&sum, &base, &ofs);
            copyVec3(&pos, &sum);
            if (variant == 0) {
                eft019_shot_set(plw, 102, 0, &pos, &plw->rot_0x54, 1.0f);
                shell_set_func_ptr->method_0x4C(plw, 2, shell_set_func_ptr);
            } else {
                eft019_shot_set(plw, 102, 0, &pos, &plw->rot_0x54, 0.8f);
                shell_set_func_ptr->method_0x4C(plw, 4, shell_set_func_ptr);
            }
        }
        break;
    }
}

/* 0x803329DC (0x284): the repeating combo act on motion slot 0. */
void pl_act_handler_11(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x451, 0, 0x5A, 0 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        plw->field_0x007 = 0;
        Pl_act_set_motion(plw, 0, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_11);
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_11);
        }
        break;
    case 2:
        if (Pl_master_ck(plw) != 0 && Pl_frame_check(plw, 0, 90.0f, 0.0f) == 1) {
            if (Pl_master_ck(plw) == 1 && plw->field_0x006 >= 5) {
                plw->field_0x006 = 0;
                plw->field_0x007 = 0;
                break;
            }
            pl_act_enter(plw, 4, 12, 0);
            break;
        }
        if (Pl_frame_check(plw, 0, 56.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        } else if (Pl_frame_check(plw, 0, 88.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        }
        if (plw->field_0x378 <= 0) {
            pl_act_enter(plw, 4, 12, 0);
            break;
        }
        if (plw->field_0x007 != 0) {
            plw->field_0x007 -= 1;
        }
        if (Pl_master_ck(plw) == 1 && (u32)pl_part_flag2_ck(plw, 12) == 1 && plw->field_0x007 == 0) {
            plw->field_0x006 += 1;
            plw->field_0x007 = 3;
        }
        pl_act_hit_react(plw, -1.0f);
        if (Pl_frame_check(plw, 0, 34.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 7, 0);
        } else if (Pl_frame_check(plw, 0, 72.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 8, 0);
        }
        break;
    }
}

/* 0x80332C60 (0xE4): the guard-hit act on motion slot 0. */
void pl_act_handler_12(_PLW* plw, s32 variant) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        if (Get_motion_no(plw) == 0x451 && Pl_frame_check(plw, 3, 38.0f, 74.0f) == 1) {
            Pl_chr_set_attr_default(plw, 0x453, 8, 0);
        } else {
            Pl_chr_set_attr_default(plw, 0x452, 8, 0);
        }
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        }
        break;
    }
}

/* 0x80332D44 (0xE4): the two-way step act on motion slot 0. */
void pl_act_handler_13(_PLW* plw, s32 variant) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        if (variant == 0) {
            Pl_chr_set_attr_default(plw, 0x4C5, 2, 0);
            Pl_act_set_step_table(plw, (u32)pl_act_steps_13a, 0);
        } else {
            Pl_chr_set_attr_default(plw, 0x4C7, 2, 0);
            Pl_act_set_step_table(plw, (u32)pl_act_steps_13b, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        }
        break;
    }
}

/* 0x80332E28 (0x124): a charged attack act on motion slot 0. */
void pl_act_handler_14(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x469, 4, 0, 0xF },
        { 0x467, 2, 0, 0, 0x469, 0, 0x12, 0xF },
        { 0x46B, 2, 0, 0, 0x469, 0, 0x12, 0xF },
        { 0xFFFF, 0, 0, 0, 0x469, 4, 4, 0xF },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 1, 0);
        plw->field_0x5C6 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_14);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_14);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80332F4C (0xA4): the stagger act on motion slot 0. */
void pl_act_handler_15(_PLW* plw) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 0, 0, 0);
        Pl_chr_set_attr_default(plw, 0x462, 4, 90);
        Pl_act_set_step_table(plw, (u32)pl_act_steps_15, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 0, 4, 0);
        }
        break;
    }
}

/* 0x80332FF0 (0x120): a three-stage attack act on motion slot 3; variant 1 also steps the act timer. */
void pl_act_handler_16(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x47F, 4, 0, 0x10 },
        { 0x48B, 4, 0, 0, 0x47F, 0, 0x32, 0x10 },
        { 0x487, 4, 0, 0, 0x47F, 0, 0x34, 0x10 },
        { 0x489, 4, 0, 0, 0x47F, 0, 0x34, 0x10 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_16);
        if (variant == 1) {
            pl_act_timer_step(plw);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_16);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333110 (0x120): an unguarded attack act on motion slot 3. */
void pl_act_handler_17(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[5] = {
        { 0xFFFF, 0, 0, 0, 0x480, 4, 0, 0x11 },
        { 0x48A, 2, 0, 0, 0x480, 0, 0x26, -30 },
        { 0x4A9, 6, 0, 0, 0x480, 0, 0x22, 0x11 },
        { 0x486, 4, 0, 0, 0x480, 0, 0x26, 0x11 },
        { 0x488, 4, 0, 0, 0x480, 0, 0x26, 0x11 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x018 = 1;
        plw->field_0x5C6 = 0;
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_17);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_17);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333230 (0x110): a one-variant attack act on motion slot 3. */
void pl_act_handler_18(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x481, 4, 0, 0x12 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_18);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_18);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333340 (0x110): a one-variant attack act on motion slot 3. */
void pl_act_handler_19(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x482, 4, 0, 0x13 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_19);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_19);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333450 (0x28C): the combo-finisher act on motion slot 3. */
void pl_act_handler_20(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x483, 4, 0, 0 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        plw->field_0x007 = 0;
        Pl_act_set_motion(plw, 3, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_20);
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_20);
        }
        break;
    case 2:
        if (Pl_master_ck(plw) != 0 && Pl_frame_check(plw, 0, 90.0f, 0.0f) == 1) {
            if (Pl_master_ck(plw) == 1 && plw->field_0x006 >= 5 && plw->field_0x378 > 0) {
                pl_act_enter(plw, 4, 0x29, 0x80);
            } else {
                pl_act_enter(plw, 4, 0x2A, 0x80);
            }
            break;
        }
        if (Pl_frame_check(plw, 0, 54.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        } else if (Pl_frame_check(plw, 0, 88.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        }
        if (plw->field_0x007 != 0) {
            plw->field_0x007 -= 1;
        }
        if (Pl_master_ck(plw) == 1 && (u32)pl_part_flag2_ck(plw, 12) == 1 && plw->field_0x007 == 0) {
            plw->field_0x006 += 1;
            plw->field_0x007 = 3;
        }
        pl_act_hit_react(plw, -1.0f);
        if (Pl_frame_check(plw, 0, 34.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 20, 0);
        } else if (Pl_frame_check(plw, 0, 72.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 21, 0);
        }
        break;
    }
}

/* 0x803336DC (0x284): the repeating combo act on motion slot 3. */
void pl_act_handler_21(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[1] = {
        { 0xFFFF, 0, 0, 0, 0x483, 0, 0x5A, 0 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        plw->field_0x007 = 0;
        Pl_act_set_motion(plw, 3, 1, 0);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_21);
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_21);
        }
        break;
    case 2:
        if (Pl_master_ck(plw) != 0 && Pl_frame_check(plw, 0, 90.0f, 0.0f) == 1) {
            if (Pl_master_ck(plw) == 1 && plw->field_0x006 >= 5) {
                plw->field_0x006 = 0;
                plw->field_0x007 = 0;
                break;
            }
            pl_act_enter(plw, 4, 0x2A, 0x80);
            break;
        }
        if (Pl_frame_check(plw, 0, 54.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        } else if (Pl_frame_check(plw, 0, 88.0f, 0.0f) == 1) {
            pl_sub_gauge_add(plw, -100);
        }
        if (plw->field_0x378 <= 0) {
            pl_act_enter(plw, 4, 0x2A, 0x80);
            break;
        }
        if (plw->field_0x007 != 0) {
            plw->field_0x007 -= 1;
        }
        if (Pl_master_ck(plw) == 1 && (u32)pl_part_flag2_ck(plw, 12) == 1 && plw->field_0x007 == 0) {
            plw->field_0x006 += 1;
            plw->field_0x007 = 3;
        }
        pl_act_hit_react(plw, -1.0f);
        if (Pl_frame_check(plw, 0, 34.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 22, 0);
        } else if (Pl_frame_check(plw, 0, 72.0f, 0.0f) == 1) {
            pl_frame_hit_set(plw, 23, 0);
        }
        break;
    }
}

/* 0x80333960 (0xE4): the guard-hit act on motion slot 3. */
void pl_act_handler_22(_PLW* plw, s32 variant) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        if (Get_motion_no(plw) == 0x483 && Pl_frame_check(plw, 3, 38.0f, 74.0f) == 1) {
            Pl_chr_set_attr_default(plw, 0x485, 8, 0);
        } else {
            Pl_chr_set_attr_default(plw, 0x484, 8, 0);
        }
        pl_act_arm_flags(plw, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        }
        break;
    }
}

/* 0x80333A44 (0xDC): moves the actor along its rotation on the three lunge motions' frame windows. */
void pl_act_lunge_slide_xyz(_PLW* plw, s32 variant) {
    VEC3 step;
    f32 start;
    f32 end;

    VEC3_ctor(&step);
    step.x = 0.0f;
    step.y = 0.0f;
    step.z = 0.0f;
    if ((u32)(variant - 5) <= 2) {
        step.z = 42.0f;
    }
    switch (Get_motion_no(plw)) {
    case 1170:
        start = 28.0f;
        end = 36.0f;
        break;
    case 1180:
        start = 18.0f;
        end = 26.0f;
        break;
    case 1182:
        start = 30.0f;
        end = 38.0f;
        break;
    default:
        return;
    }
    if (Pl_frame_check(plw, 3, start, end) == 1) {
        rotVecXYZ(&step, &plw->rot_0x54);
        addVec3To(&plw->vec_0x03C, &step);
    }
}

/* 0x80333B20 (0x16C): the lunge attack act on motion slot 3. */
void pl_act_handler_23(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[9] = {
        { 0xFFFF, 0, 0, 0, 0x492, 4, 0, 0x18 },
        { 0x4A8, 6, 0, 0, 0x492, 0, 0x24, 0x18 },
        { 0x49A, 4, 0, 0, 0x492, 0, 0x24, 0x18 },
        { 0x49C, 6, 0, 0, 0x492, 0, 0x26, 0x18 },
        { 0x49E, 4, 0, 0, 0x492, 0, 0x26, 0x18 },
        { 0x49E, 4, 0, 0, 0x492, 0, 0x26, 0x18 },
        { 0xFFFF, 0, 0, 0, 0x492, 4, 0, 0x18 },
        { 0x49C, 6, 0, 0, 0x492, 0, 0x26, 0x18 },
        { 0x4A8, 6, 0, 0, 0x492, 0, 0x24, -24 },
    };
    VEC3 unused_pos;

    VEC3_ctor(&unused_pos);

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x5C6 = 1;
        plw->field_0x018 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_clear_flag5bb(plw);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_23);
        if (variant == 2) {
            pl_act_timer_step(plw);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_23);
        }
        pl_act_lunge_slide_xyz(plw, variant);
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
            pl_act_lunge_slide_xyz(plw, variant);
        }
        break;
    }
}

/* 0x80333C8C (0x124): a charged attack act on motion slot 3. */
void pl_act_handler_24(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[3] = {
        { 0xFFFF, 0, 0, 0, 0x493, 4, 0, 0x19 },
        { 0x498, 2, 0, 0, 0x493, 0, 0x2C, 0x19 },
        { 0xFFFF, 0, 0, 0, 0x493, 0xE, 0x2C, 0x19 },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x5C6 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_24);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_24);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333DB0 (0x124): a charged attack act on motion slot 3. */
void pl_act_handler_25(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x49B, 4, 0, 0x1C },
        { 0x499, 2, 0, 0, 0x49B, 0, 0x16, 0x1C },
        { 0x49D, 6, 0, 0, 0x49B, 0, 0x16, 0x1C },
        { 0xFFFF, 0, 0, 0, 0x49B, 4, 4, 0x1C },
    };

    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x5C6 = 1;
        pl_act_add_charge(plw, -4);
        pl_act_set_motion_gate(plw, 70);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_25);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_25);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        break;
    }
}

/* 0x80333ED4 (0xE4): the two-way step act on motion slot 3. */
void pl_act_handler_26(_PLW* plw, s32 variant) {
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        if (variant == 0) {
            Pl_chr_set_attr_default(plw, 0x4F7, 2, 0);
            Pl_act_set_step_table(plw, (u32)pl_act_steps_26a, 0);
        } else {
            Pl_chr_set_attr_default(plw, 0x4F7, 2, 0);
            Pl_act_set_step_table(plw, (u32)pl_act_steps_26b, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        }
        break;
    }
}

/* 0x80333FB8 (0x1F8): the impact act on motion slot 3. */
void pl_act_handler_27(_PLW* plw, s32 variant) {
    static PlActAttrRow rows[4] = {
        { 0xFFFF, 0, 0, 0, 0x494, 6, 0, 0x1A },
        { 0xFFFF, 0, 0, 0, 0x494, 6, 0, 0x1A },
        { 0xFFFF, 0, 0, 0, 0x494, 6, 0, 0x1A },
        { 0xFFFF, 0, 0, 0, 0x494, -26, 0x18, 0x1A },
    };
    VEC3 pos;
    s32 step;

    VEC3_ctor(&pos);
    pl_act_set_motion_gate(plw, 2);
    step = plw->act_step_0x05;
    switch (step) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x5C6 = 1;
        plw->field_0x310 = 0;
        pl_act_add_charge(plw, -11);
        pl_act_attr_stage_set(plw, 0, &rows[variant], pl_act_steps_27);
        if (variant == 3) {
            pl_impact_pos_get(plw, &pos);
            pl_impact_se_req(plw, &pos);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            plw->act_step_0x05 += 1;
            pl_act_attr_stage_set(plw, 1, &rows[variant], pl_act_steps_27);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else {
            pl_act_hit_react(plw, -1.0f);
        }
        if (Pl_frame_check(plw, 0, 42.0f, 0.0f) == 1) {
            setVector3(&pos, -23.0f, 6.0f, 21.0f);
            eft007_set(plw, 4, 0, 11, &pos, 1.0f);
        }
        if (Pl_frame_check(plw, 0, 78.0f, 0.0f) == 1) {
            setVector3(&pos, -33.0f, 12.0f, 18.0f);
            eft007_set(plw, 6, 0, 11, &pos, 1.0f);
        }
        break;
    }
}

/* 0x803341B0 (0x1E8): the charge act on motion slot 3. */
void pl_act_handler_28(_PLW* plw) {
    pl_act_set_motion_gate(plw, 2);
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        plw->field_0x006 = 0;
        Pl_act_set_motion(plw, 3, 1, 0);
        plw->field_0x5C6 = 1;
        Pl_chr_setX(plw, 0x495, 6, 0);
        Pl_act_set_step_table(plw, (u32)pl_act_steps_28, 0);
        plw->field_0x28 = 30;
        plw->charge_gauge_0x310 += 1;
        plw->count_0x034 = 0;
        pl_frame_hit_set(plw, 27, 0);
        pl_act_add_charge(plw, -3);
        break;
    case 1:
        if (Pl_master_ck(plw) == 1) {
            if (plw->field_0x007 != 0) {
                plw->field_0x007 -= 1;
            }
            if ((u32)pl_part_flag2_ck(plw, 7) == 1 && plw->field_0x007 == 0) {
                plw->field_0x006 += 1;
                plw->field_0x007 = 3;
            }
            if (--plw->field_0x28 <= 0) {
                if (plw->field_0x006 >= 4 && pl_gauge_level_get(plw) <= 2) {
                    if (plw->charge_gauge_0x310 >= 2) {
                        pl_act_enter(plw, 4, 0x38, 0x80);
                    } else {
                        pl_act_enter(plw, 4, 0x37, 0x80);
                    }
                } else {
                    pl_act_enter(plw, 4, 0x4B, 0x80);
                }
            }
        }
        if (++plw->count_0x034 >= 10) {
            plw->count_0x034 = 0;
            pl_frame_hit_set(plw, 27, 0);
        }
        break;
    }
}

/* 0x80334398 (0x1D0): the shot act on motion slot 3. */
void pl_act_handler_29(_PLW* plw, s32 variant) {
    VEC3 pos;
    VEC3 base;
    VEC3 ofs;
    VEC3 sum;

    VEC3_ctor(&pos);
    VEC3_ctor(&base);
    VEC3_ctor(&ofs);
    switch (plw->act_step_0x05) {
    case 0:
        plw->act_step_0x05 += 1;
        Pl_act_set_motion(plw, 3, 1, 0);
        Pl_chr_set_attr_default(plw, 0x497, 2, 0);
        Pl_act_set_step_table(plw, (u32)pl_act_steps_29, 0);
        pl_act_set_motion_gate(plw, 70);
        plw->field_0x5C6 = 0;
        pl_act_add_charge(plw, -20);
        break;
    case 1:
        if (Pl_motion_end_ck(plw) == 1) {
            Pl_act_set_motion_slot(plw, 3, 4, 0);
        } else if (Pl_frame_check(plw, 0, 12.0f, 0.0f) == 1) {
            copyVec3(&base, &plw->vec_0x03C);
            ofs.x = -30.0f;
            ofs.y = 180.0f;
            ofs.z = 155.0f;
            rotVecXYZ(&ofs, &plw->rot_0x54);
            addVec3(&sum, &base, &ofs);
            copyVec3(&pos, &sum);
            if (variant == 0) {
                eft019_shot_set(plw, 102, 0, &pos, &plw->rot_0x54, 1.0f);
                shell_set_func_ptr->method_0x4C(plw, 3, shell_set_func_ptr);
            } else {
                eft019_shot_set(plw, 102, 0, &pos, &plw->rot_0x54, 0.8f);
                shell_set_func_ptr->method_0x4C(plw, 5, shell_set_func_ptr);
            }
        }
        break;
    }
}

} /* extern "C" */

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
        Pl_act_set_step_table(plw, (u32)pl_act_steps_table_enter, 0);
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

u16 pl_act_steps_01[0x42] = {
    0x0400, 0x001A, 0x1000, 0x0019, 0x000B, 0x0000, 0x0000, 0x0000,
    0x0050, 0x0064, 0x0000, 0x0400, 0x001A, 0x1000, 0x0019, 0x000A,
    0x0000, 0x0000, 0x0000, 0x0050, 0x0064, 0x0000, 0x0000, 0x000F,
    0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x0050, 0x0000,
    0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000,
    0x0001, 0x0050, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x0050, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_act_steps_02[0x42] = {
    0x0400, 0x0008, 0x0000, 0x0019, 0x000C, 0x0000, 0x0000, 0x0000,
    0x003A, 0x0044, 0x0014, 0x0400, 0x001D, 0x1000, 0x0019, 0x000A,
    0x0000, 0x0000, 0x0000, 0x004A, 0x005E, 0x0000, 0x0000, 0x000F,
    0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x004A, 0x0000,
    0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000,
    0x0001, 0x004A, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x004A, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_act_steps_03[0x58] = {
    0x0400, 0x0046, 0x0000, 0x8019, 0x0005, 0x0000, 0x0000, 0x0035,
    0x0030, 0x004E, 0x0000, 0x0400, 0x000F, 0x0000, 0x0019, 0x0005,
    0x0000, 0x0000, 0x0000, 0x0030, 0x004E, 0x0000, 0x0400, 0x001B,
    0x1000, 0x0019, 0x000B, 0x0000, 0x0000, 0x0000, 0x0030, 0x004E,
    0x0000, 0x0400, 0x001C, 0x1000, 0x0019, 0x000A, 0x0000, 0x0000,
    0x0000, 0x0030, 0x004E, 0x0000, 0x0000, 0x000F, 0x0000, 0x8016,
    0x003E, 0x0000, 0x0000, 0x0001, 0x0040, 0x0000, 0x0000, 0x0000,
    0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000, 0x0001, 0x0040,
    0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x0040, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_04[0x58] = {
    0x0400, 0x0009, 0x1000, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x0038, 0x0054, 0x0000, 0x0400, 0x001E, 0x1000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x002D, 0x0038, 0x0054, 0x0000, 0x0400, 0x001E,
    0x1000, 0x8019, 0x0018, 0x0000, 0x0000, 0x002D, 0x0038, 0x0054,
    0x0000, 0x0400, 0x001F, 0x1000, 0x8019, 0x0012, 0x0000, 0x0000,
    0x002D, 0x0038, 0x0054, 0x0000, 0x0000, 0x000F, 0x0000, 0x8016,
    0x003E, 0x0000, 0x0000, 0x0001, 0x0038, 0x0000, 0x0000, 0x0000,
    0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000, 0x0001, 0x0038,
    0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x0038, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_05[0x79] = {
    0x0400, 0x0048, 0x1000, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x0042, 0x0060, 0x0000, 0x0400, 0x004D, 0x1000, 0x0000, 0x0000,
    0x0000, 0x2000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0400, 0x004D,
    0x1000, 0x801A, 0x0015, 0x0000, 0x2000, 0x002D, 0x0042, 0x0060,
    0x0000, 0x0400, 0x0021, 0x1000, 0x8019, 0x0015, 0x0000, 0x0000,
    0x002D, 0x0042, 0x0060, 0x0000, 0x0400, 0x0021, 0x1000, 0x8019,
    0x0018, 0x0000, 0x0000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0400,
    0x0021, 0x1000, 0x8019, 0x0019, 0x0000, 0x0000, 0x002D, 0x0042,
    0x0060, 0x0000, 0x0400, 0x0020, 0x1000, 0x8019, 0x0012, 0x0000,
    0x0000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0000, 0x000F, 0x0000,
    0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x0050, 0x0000, 0x0000,
    0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000, 0x0001,
    0x0050, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016, 0x0019,
    0x0000, 0x0000, 0x0001, 0x0050, 0x0000, 0x0000, 0xFF00, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,
};
u16 pl_act_steps_06[0x42] = {
    0x0400, 0x0004, 0x1000, 0x001D, 0x000F, 0x0000, 0x0000, 0x0000,
    0x003C, 0x0050, 0x0000, 0x0400, 0x002B, 0x1000, 0x0019, 0x0013,
    0x0000, 0x0000, 0x0000, 0x003C, 0x0050, 0x0000, 0x0000, 0x000F,
    0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x003C, 0x0000,
    0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000,
    0x0001, 0x003C, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x003C, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_act_steps_07[0x2C] = {
    0x0000, 0x000F, 0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001,
    0x0096, 0x0000, 0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F,
    0x0000, 0x0000, 0x0001, 0x0096, 0x0000, 0x0000, 0x0000, 0x0008,
    0x0000, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001, 0x0096, 0x0000,
    0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_08[0x42] = {
    0x0400, 0x0018, 0x0000, 0x8019, 0x0007, 0x0000, 0x0000, 0x002D,
    0x0052, 0x0066, 0x0000, 0x0400, 0x0049, 0x1000, 0x8019, 0x0016,
    0x0000, 0x0000, 0x002D, 0x0078, 0x008C, 0x0000, 0x0000, 0x000F,
    0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x0078, 0x0000,
    0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000,
    0x0001, 0x0078, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x0078, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_act_steps_15[0x37] = {
    0x0400, 0x0049, 0x1000, 0x8019, 0x0016, 0x0000, 0x0000, 0x002D,
    0x0078, 0x008C, 0x0000, 0x0000, 0x000F, 0x0000, 0x8016, 0x003E,
    0x0000, 0x0000, 0x0001, 0x0078, 0x0000, 0x0000, 0x0000, 0x0010,
    0x0000, 0x8016, 0x003F, 0x0000, 0x0000, 0x0001, 0x0078, 0x0000,
    0x0000, 0x0000, 0x0008, 0x0000, 0x8016, 0x0019, 0x0000, 0x0000,
    0x0001, 0x0078, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_09[0x16] = {
    0x0400, 0x0053, 0x0000, 0x8019, 0x0008, 0x0000, 0x0000, 0x002D,
    0x0014, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_10[0x2C] = {
    0x0000, 0x000F, 0x0000, 0x8016, 0x003E, 0x0000, 0x0000, 0x0001,
    0x00A0, 0x0000, 0x0000, 0x0000, 0x0010, 0x0000, 0x8016, 0x003F,
    0x0000, 0x0000, 0x0001, 0x00A0, 0x0000, 0x0000, 0x0000, 0x0008,
    0x0000, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001, 0x00A0, 0x0000,
    0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_11[0xB] = {
    0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_13a[0x16] = {
    0x0400, 0x0013, 0x4000, 0x0019, 0x000D, 0x0000, 0x0000, 0x0000,
    0x0012, 0x001E, 0x0010, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_13b[0x16] = {
    0x0400, 0x0014, 0x4000, 0x0019, 0x000D, 0x0000, 0x0000, 0x0000,
    0x0012, 0x001E, 0x0010, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_14[0x79] = {
    0x0400, 0x0056, 0x1000, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x0028, 0x003A, 0x0000, 0x0400, 0x0017, 0x0000, 0x0019, 0x0005,
    0x0000, 0x0000, 0x0000, 0x0028, 0x003A, 0x0000, 0x0400, 0x0023,
    0x1000, 0x801A, 0x001C, 0x0000, 0x2000, 0x002D, 0x0028, 0x003A,
    0x0000, 0x0400, 0x0023, 0x1000, 0x801A, 0x0015, 0x0000, 0x2000,
    0x002D, 0x0028, 0x003A, 0x0000, 0x0400, 0x0022, 0x1000, 0x8019,
    0x0015, 0x0000, 0x0000, 0x002D, 0x0028, 0x003A, 0x0000, 0x0400,
    0x0022, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000, 0x002D, 0x0028,
    0x003A, 0x0000, 0x0400, 0x0022, 0x1000, 0x8019, 0x001D, 0x0000,
    0x0000, 0x002D, 0x0028, 0x003A, 0x0000, 0x0000, 0x000F, 0x0000,
    0x8016, 0x003E, 0x0000, 0x0000, 0x0001, 0x0032, 0x0000, 0x0000,
    0x0000, 0x0010, 0x0000, 0x8016, 0x003F, 0x0000, 0x0000, 0x0001,
    0x0032, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x8016, 0x0019,
    0x0000, 0x0000, 0x0001, 0x0032, 0x0000, 0x0000, 0xFF00, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000,
};
u16 pl_act_steps_16[0x58] = {
    0x0400, 0x0042, 0x1080, 0x0019, 0x000B, 0x0000, 0x0000, 0x0000,
    0x005A, 0x006E, 0x0000, 0x0400, 0x0042, 0x1080, 0x0019, 0x000A,
    0x0000, 0x0000, 0x0000, 0x005A, 0x006E, 0x0000, 0x0000, 0x0064,
    0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017, 0x005A, 0x0000,
    0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000,
    0x0001, 0x005A, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016,
    0x0044, 0x0000, 0x0000, 0x0001, 0x005A, 0x0000, 0x0000, 0x0000,
    0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x005A,
    0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x005A, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_17[0x58] = {
    0x0400, 0x0027, 0x1080, 0x001D, 0x000F, 0x0000, 0x0000, 0x0000,
    0x003C, 0x0050, 0x0000, 0x0400, 0x0026, 0x1080, 0x0019, 0x0013,
    0x0000, 0x0000, 0x0000, 0x003C, 0x0050, 0x0000, 0x0000, 0x0064,
    0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017, 0x003C, 0x0000,
    0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000,
    0x0001, 0x003C, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016,
    0x0044, 0x0000, 0x0000, 0x0001, 0x003C, 0x0000, 0x0000, 0x0000,
    0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x003C,
    0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x003C, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_18[0x58] = {
    0x0400, 0x0028, 0x0080, 0x0019, 0x000C, 0x0000, 0x0000, 0x0000,
    0x003A, 0x0044, 0x0014, 0x0400, 0x0045, 0x1080, 0x0019, 0x000A,
    0x0000, 0x0000, 0x0000, 0x005A, 0x006E, 0x0000, 0x0000, 0x0064,
    0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017, 0x003A, 0x0000,
    0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000,
    0x0001, 0x003A, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016,
    0x0044, 0x0000, 0x0000, 0x0001, 0x003A, 0x0000, 0x0000, 0x0000,
    0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x003A,
    0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x003A, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_19[0x6E] = {
    0x0400, 0x0047, 0x0080, 0x8019, 0x0005, 0x0000, 0x0000, 0x0035,
    0x0038, 0x0056, 0x0000, 0x0400, 0x003C, 0x0080, 0x0019, 0x0005,
    0x0000, 0x0000, 0x0000, 0x0038, 0x0056, 0x0000, 0x0400, 0x0043,
    0x1080, 0x0019, 0x000B, 0x0000, 0x0000, 0x0000, 0x0038, 0x0056,
    0x0000, 0x0400, 0x0044, 0x1080, 0x0019, 0x000A, 0x0000, 0x0000,
    0x0000, 0x0038, 0x0056, 0x0000, 0x0000, 0x0064, 0x0080, 0x8016,
    0x0040, 0x0000, 0x0000, 0x0017, 0x0038, 0x0000, 0x0000, 0x0000,
    0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000, 0x0001, 0x0038,
    0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016, 0x0044, 0x0000,
    0x0000, 0x0001, 0x0038, 0x0000, 0x0000, 0x0000, 0x001A, 0x0080,
    0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x0038, 0x0000, 0x0000,
    0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001,
    0x0038, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_20[0x42] = {
    0x0000, 0x0064, 0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017,
    0x0096, 0x0000, 0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041,
    0x0000, 0x0000, 0x0001, 0x0096, 0x0000, 0x0000, 0x0000, 0x0019,
    0x0080, 0x8016, 0x0044, 0x0000, 0x0000, 0x0001, 0x0096, 0x0000,
    0x0000, 0x0000, 0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000,
    0x0001, 0x0096, 0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x0096, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_act_steps_21[0xB] = {
    0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_23[0x6E] = {
    0x0400, 0x0035, 0x1080, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x0038, 0x0054, 0x0000, 0x0400, 0x0039, 0x1080, 0x0000, 0x0000,
    0x0000, 0x0000, 0x002D, 0x0038, 0x0054, 0x0000, 0x0400, 0x0039,
    0x1080, 0x8019, 0x0018, 0x0000, 0x0000, 0x002D, 0x0038, 0x0054,
    0x0000, 0x0400, 0x003A, 0x1080, 0x8019, 0x0012, 0x0000, 0x0000,
    0x002D, 0x0038, 0x0054, 0x0000, 0x0000, 0x0064, 0x0080, 0x8016,
    0x0040, 0x0000, 0x0000, 0x0017, 0x0038, 0x0000, 0x0000, 0x0000,
    0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000, 0x0001, 0x0038,
    0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016, 0x0044, 0x0000,
    0x0000, 0x0001, 0x0038, 0x0000, 0x0000, 0x0000, 0x001A, 0x0080,
    0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x0038, 0x0000, 0x0000,
    0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001,
    0x0038, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_24[0x8F] = {
    0x0400, 0x0057, 0x1080, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x0042, 0x0060, 0x0000, 0x0400, 0x004F, 0x1080, 0x0000, 0x0000,
    0x0000, 0x2000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0400, 0x004F,
    0x1080, 0x801A, 0x0015, 0x0000, 0x2000, 0x002D, 0x0042, 0x0060,
    0x0000, 0x0400, 0x003D, 0x1080, 0x8019, 0x0015, 0x0000, 0x0000,
    0x002D, 0x0042, 0x0060, 0x0000, 0x0400, 0x003D, 0x1080, 0x8019,
    0x0018, 0x0000, 0x0000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0400,
    0x003D, 0x1080, 0x8019, 0x0019, 0x0000, 0x0000, 0x002D, 0x0042,
    0x0060, 0x0000, 0x0400, 0x003B, 0x1080, 0x8019, 0x0012, 0x0000,
    0x0000, 0x002D, 0x0042, 0x0060, 0x0000, 0x0000, 0x0064, 0x0080,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0017, 0x0050, 0x0000, 0x0000,
    0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000, 0x0001,
    0x0050, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0001, 0x0050, 0x0000, 0x0000, 0x0000, 0x001A,
    0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x0050, 0x0000,
    0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000,
    0x0001, 0x0050, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_25[0x8F] = {
    0x0400, 0x0058, 0x1080, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D,
    0x002C, 0x003E, 0x0000, 0x0400, 0x0041, 0x0080, 0x0019, 0x0005,
    0x0000, 0x0000, 0x0000, 0x002C, 0x003E, 0x0000, 0x0400, 0x003F,
    0x1080, 0x801A, 0x001C, 0x0000, 0x2000, 0x002D, 0x002C, 0x003E,
    0x0000, 0x0400, 0x003F, 0x1080, 0x801A, 0x0015, 0x0000, 0x2000,
    0x002D, 0x002C, 0x003E, 0x0000, 0x0400, 0x003E, 0x1080, 0x8019,
    0x0015, 0x0000, 0x0000, 0x002D, 0x002C, 0x003E, 0x0000, 0x0400,
    0x003E, 0x1080, 0x0000, 0x0000, 0x0000, 0x0000, 0x002D, 0x002C,
    0x003E, 0x0000, 0x0400, 0x003E, 0x1080, 0x8019, 0x001D, 0x0000,
    0x0000, 0x002D, 0x002C, 0x003E, 0x0000, 0x0000, 0x0064, 0x0080,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0017, 0x002C, 0x0000, 0x0000,
    0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000, 0x0001,
    0x002C, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0001, 0x002C, 0x0000, 0x0000, 0x0000, 0x001A,
    0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x002C, 0x0000,
    0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000,
    0x0001, 0x002C, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_26a[0x16] = {
    0x0400, 0x0033, 0x4080, 0x0019, 0x000D, 0x0000, 0x0000, 0x0000,
    0x0012, 0x001E, 0x0010, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_26b[0x16] = {
    0x0400, 0x0034, 0x4080, 0x0019, 0x000D, 0x0000, 0x0000, 0x0000,
    0x0012, 0x001E, 0x0010, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_27[0x58] = {
    0x0400, 0x0037, 0x0080, 0x8019, 0x0007, 0x0000, 0x0000, 0x002D,
    0x0052, 0x0066, 0x0000, 0x0400, 0x004A, 0x1080, 0x8019, 0x0016,
    0x0000, 0x0000, 0x002D, 0x0088, 0x009C, 0x0000, 0x0000, 0x0064,
    0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017, 0x0088, 0x0000,
    0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041, 0x0000, 0x0000,
    0x0001, 0x0088, 0x0000, 0x0000, 0x0000, 0x0019, 0x0080, 0x8016,
    0x0044, 0x0000, 0x0000, 0x0001, 0x0088, 0x0000, 0x0000, 0x0000,
    0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000, 0x0001, 0x0088,
    0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000,
    0x0000, 0x0001, 0x0088, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_table_enter[0x4D] = {
    0x0400, 0x004A, 0x1080, 0x8019, 0x0016, 0x0000, 0x0000, 0x002D,
    0x0088, 0x009C, 0x0000, 0x0000, 0x0064, 0x0080, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0017, 0x0088, 0x0000, 0x0000, 0x0000, 0x0065,
    0x0080, 0x8016, 0x0041, 0x0000, 0x0000, 0x0001, 0x0088, 0x0000,
    0x0000, 0x0000, 0x0019, 0x0080, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0088, 0x0000, 0x0000, 0x0000, 0x001A, 0x0080, 0x8016,
    0x0045, 0x0000, 0x0000, 0x0001, 0x0088, 0x0000, 0x0000, 0x0000,
    0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001, 0x0088,
    0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_28[0x16] = {
    0x0400, 0x0054, 0x0080, 0x8019, 0x0008, 0x0000, 0x0000, 0x002D,
    0x0014, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_act_steps_29[0x42] = {
    0x0000, 0x0064, 0x0080, 0x8016, 0x0040, 0x0000, 0x0000, 0x0017,
    0x00A0, 0x0000, 0x0000, 0x0000, 0x0065, 0x0080, 0x8016, 0x0041,
    0x0000, 0x0000, 0x0001, 0x00A0, 0x0000, 0x0000, 0x0000, 0x0019,
    0x0080, 0x8016, 0x0044, 0x0000, 0x0000, 0x0001, 0x00A0, 0x0000,
    0x0000, 0x0000, 0x001A, 0x0080, 0x8016, 0x0045, 0x0000, 0x0000,
    0x0001, 0x00A0, 0x0000, 0x0000, 0x0000, 0x001B, 0x0080, 0x8016,
    0x0019, 0x0000, 0x0000, 0x0001, 0x00A0, 0x0000, 0x0000, 0xFF00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000,
};
u16 pl_flinch_steps_kind8[0x16] = {
    0x0400, 0x0052, 0x1000, 0x8019, 0x001F, 0x0000, 0x0000, 0x002D,
    0x0032, 0x0000, 0x000A, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_flinch_steps_kind8_guard[0x16] = {
    0x0400, 0x0055, 0x1080, 0x8019, 0x001F, 0x0000, 0x0000, 0x002D,
    0x0044, 0x0000, 0x000A, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_steps_kind8[0xE7] = {
    0x8900, 0x00F8, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002F,
    0x0000, 0x0000, 0x0000, 0x0600, 0x001B, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x0012, 0x0000, 0x0000, 0x0000, 0x0600, 0x002B,
    0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002A, 0x0000, 0x0000,
    0x0000, 0x0600, 0x003F, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000,
    0x002B, 0x0000, 0x0000, 0x0000, 0x0000, 0x0014, 0x0010, 0x8000,
    0x0000, 0x0000, 0x0000, 0x0027, 0x0000, 0x0000, 0x0000, 0x0000,
    0x001E, 0x0000, 0x8000, 0x0003, 0x0000, 0x0000, 0x0024, 0x0000,
    0x0000, 0x0000, 0x0000, 0x00A6, 0x0000, 0x8016, 0x0034, 0x0000,
    0x0000, 0x0031, 0x0000, 0x0000, 0x0000, 0x0000, 0x00B1, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0032, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0004, 0x0000, 0x0017, 0x000E, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x0000, 0x0016, 0x000D,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0046,
    0x0000, 0x8019, 0x0000, 0x0000, 0x0000, 0x0035, 0x0000, 0x0000,
    0x0000, 0x0400, 0x0001, 0x0000, 0x0019, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0007, 0x0000, 0x0019,
    0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
    0x0003, 0x0000, 0x0019, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0400, 0x0004, 0x0000, 0x001D, 0x000E, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0002, 0x0000,
    0x0019, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0008, 0x0000, 0x8016, 0x0004, 0x0000, 0x0000, 0x0001,
    0x0000, 0x0000, 0x0000, 0x8B00, 0x00FA, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x002E, 0x0000, 0x0000, 0x0000, 0x0100, 0x000A,
    0x0000, 0xC009, 0x0000, 0x0000, 0x0000, 0x4024, 0x0000, 0x0000,
    0x0000, 0x0100, 0x0000, 0x0000, 0xC009, 0x0000, 0x0000, 0x0000,
    0x4002, 0x0000, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_steps_kind8_guard[0x108] = {
    0x8900, 0x00F8, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002F,
    0x0000, 0x0000, 0x0000, 0x0600, 0x001B, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x0012, 0x0000, 0x0000, 0x0000, 0x0600, 0x002B,
    0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002A, 0x0000, 0x0000,
    0x0000, 0x0600, 0x003F, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000,
    0x002B, 0x0000, 0x0000, 0x0000, 0x0400, 0x0000, 0x0000, 0x8000,
    0x0000, 0x0000, 0x0000, 0x002C, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0014, 0x0010, 0x8000, 0x0000, 0x0000, 0x0000, 0x0027, 0x0000,
    0x0000, 0x0000, 0x0000, 0x001E, 0x0000, 0x8000, 0x0003, 0x0000,
    0x0000, 0x0024, 0x0000, 0x0000, 0x0000, 0x0000, 0x00A6, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0031, 0x0000, 0x0000, 0x0000,
    0x0000, 0x00B1, 0x0000, 0x8016, 0x0034, 0x0000, 0x0000, 0x0032,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0004, 0x0000, 0x0017, 0x000E,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003,
    0x0000, 0x0016, 0x000D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0400, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0006, 0x0000, 0x8019,
    0x0003, 0x0000, 0x0000, 0x002D, 0x0000, 0x0000, 0x0000, 0x0400,
    0x004C, 0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x002D, 0x0000,
    0x0000, 0x0000, 0x0400, 0x0005, 0x0000, 0x8019, 0x0014, 0x0000,
    0x0000, 0x002D, 0x0000, 0x0000, 0x0000, 0x0400, 0x004C, 0x0000,
    0x801A, 0x001B, 0x0000, 0x2000, 0x002D, 0x0000, 0x0000, 0x0000,
    0x0400, 0x0005, 0x0000, 0x8019, 0x0017, 0x0000, 0x0000, 0x002D,
    0x0000, 0x0000, 0x0000, 0x0400, 0x0015, 0x0000, 0x8019, 0x0011,
    0x0000, 0x0000, 0x002D, 0x0000, 0x0000, 0x0000, 0x0400, 0x0016,
    0x1000, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0008, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0000, 0x0000, 0x0000, 0x8B00, 0x00FA, 0x0000, 0x8000,
    0x0000, 0x0000, 0x0000, 0x002E, 0x0000, 0x0000, 0x0000, 0x0100,
    0x000A, 0x0000, 0xC009, 0x0000, 0x0000, 0x0000, 0x4024, 0x0000,
    0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0xC009, 0x0000, 0x0000,
    0x0000, 0x4002, 0x0000, 0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
u16 pl_steps_kind8_slot3[0xBB] = {
    0x8900, 0x00F7, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002F,
    0x0000, 0x0000, 0x0000, 0x0600, 0x0023, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x0012, 0x0000, 0x0000, 0x0000, 0x0000, 0x009E,
    0x0010, 0x8000, 0x0000, 0x0000, 0x0000, 0x0027, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0053, 0x0000, 0x0017, 0x000E, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x002E, 0x0000, 0x0016,
    0x000D, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400,
    0x0047, 0x0080, 0x0000, 0x0000, 0x0000, 0x0000, 0x0035, 0x0000,
    0x0000, 0x0000, 0x0400, 0x002D, 0x0080, 0x0019, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0025, 0x0080,
    0x0019, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0400, 0x0026, 0x0080, 0x0019, 0x0004, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0400, 0x0027, 0x0080, 0x001D, 0x000E,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0024,
    0x0080, 0x0019, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0017, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0016, 0x0000, 0x0000, 0x0000, 0x0000, 0x0018, 0x0000, 0x001C,
    0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001, 0x0000,
    0x0000, 0x0000, 0x8B00, 0x00F9, 0x0000, 0x8000, 0x0000, 0x0000,
    0x0000, 0x002E, 0x0000, 0x0000, 0x0000, 0x0100, 0x0006, 0x0080,
    0xC009, 0x0000, 0x0000, 0x0000, 0x401C, 0x0000, 0x0000, 0x0000,
    0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,
};
u16 pl_steps_kind8_slot3_guard[0xDC] = {
    0x8900, 0x00F7, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002F,
    0x0000, 0x0000, 0x0000, 0x0600, 0x0023, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x0012, 0x0000, 0x0000, 0x0000, 0x0400, 0x002C,
    0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x002C, 0x0000, 0x0000,
    0x0000, 0x0000, 0x009E, 0x0010, 0x8000, 0x0000, 0x0000, 0x0000,
    0x0027, 0x0000, 0x0000, 0x0000, 0x0000, 0x0053, 0x0000, 0x0017,
    0x000E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x002E, 0x0000, 0x0016, 0x000D, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0400, 0x002C, 0x0080, 0x0019, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x002F, 0x0080,
    0x8019, 0x0003, 0x0000, 0x0000, 0x002D, 0x0000, 0x0000, 0x0000,
    0x0400, 0x004E, 0x0080, 0x801A, 0x0014, 0x0000, 0x2000, 0x002D,
    0x0000, 0x0000, 0x0000, 0x0400, 0x002E, 0x0080, 0x0000, 0x0000,
    0x0000, 0x0000, 0x002D, 0x0000, 0x0000, 0x0000, 0x0400, 0x004E,
    0x0080, 0x801A, 0x001B, 0x0000, 0x2000, 0x002D, 0x0000, 0x0000,
    0x0000, 0x0400, 0x002E, 0x0080, 0x8019, 0x0017, 0x0000, 0x0000,
    0x002D, 0x0000, 0x0000, 0x0000, 0x0400, 0x0030, 0x0080, 0x8019,
    0x0011, 0x0000, 0x0000, 0x002D, 0x0000, 0x0000, 0x0000, 0x0400,
    0x0036, 0x0080, 0x8019, 0x0006, 0x0000, 0x0000, 0x002D, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0017, 0x0000, 0x801C, 0x0020, 0x0000,
    0x0000, 0x0016, 0x0000, 0x0000, 0x0000, 0x0000, 0x0018, 0x0000,
    0x001C, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x001B, 0x0080, 0x8016, 0x0019, 0x0000, 0x0000, 0x0001,
    0x0000, 0x0000, 0x0000, 0x8B00, 0x00F9, 0x0000, 0x8000, 0x0000,
    0x0000, 0x0000, 0x002E, 0x0000, 0x0000, 0x0000, 0x0100, 0x0006,
    0x0080, 0xC009, 0x0000, 0x0000, 0x0000, 0x401C, 0x0000, 0x0000,
    0x0000, 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

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
            quest_net_kill_apply(msg->status.value_0x06, msg->status.enemy_id, msg->status.value_0x07, 0,
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

