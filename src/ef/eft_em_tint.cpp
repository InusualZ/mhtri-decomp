/* ef/eft_em_tint.cpp - the debris effect (`eft_em_tint_*`, effect type 0x28): the tinted spawns the enemy part-break
 *   bands call, the record's create, release and dispatch, and its create/move/end/kill steps; then the area effect's
 *   creator, release and dispatcher (type 0x29, `lb_area_fx_set`, `eft041_*`).
 * RANGE. .text 0x80305924-0x803066F0 (16 functions, defined in address order); .sdata 0x80792AA0-0x80792B70 (the
 *   `eft_tint_*` tables), .sdata2 0x8079ADB8-0x8079ADE0, extab 0x80015A7C-0x80015AAC, extabindex 0x80034620-0x80034668.
 *   The seam at 0x80305924: `.sdata2` holds 0.0f at 0x8079ADA8 (`eft_em_create`, `eft_em_move`) and again at
 *   0x8079ADC8 (`eft_em_tint_model_create`), and the int->float double at 0x8079ADA0 (`lb_barter_gauge_draw`) and
 *   again at 0x8079ADC0 (`eft_em_tint_create`); one TU pools a value once.
 * FLAGS. `cflags_lobby`; file-scope `#pragma peephole off`, `pool_data off`, `optimization_level 4` and
 *   `fp_contract off` (retail keeps `fmuls`+`fadds` in `eft_em_tint_create`).
 * NAMES. No `__FILE__` string and no runtime-dump name covers the range.  GUESS (from each body, its callers and the
 *   effect type it seeds): the unit name, `eft_em_tint_*` ("tint" from the spawn's colour word; the pieces are flung
 *   debris models), `eft041_release`/`eft041_dispatch`, `lb_area_fx_set` and the `eft_tint_*` tables.
 * RESIDUALS.
 *   - the area effect's creator, release and dispatcher (0x80306524-0x803066F0) are the head of the eft041 TU
 *     (`ef/fn_803066F0.c`'s `eft041_model_create`, `ef/fn_8030681C.cpp`'s `eft041_move`..`eft041_kill`): the
 *     dispatcher and its four state bodies are one family in every other effect of the band; no pool word pins them
 *     (seam request seam-eft-em#1).
 *   - `eft_em_tint_model_create`: retail loads the speed table's row pointer (r5) before the `0.0f` arguments and
 *     keeps the type in r4, ours the reverse; the case-0 loop's counter, piece pointer and part tables take
 *     r17/r14/r18,r20,r21 in retail against r18/r17/r20,r21,r14.  No gain from a row-pointer or float local,
 *     fall-before-speed, the inverted group test, an `s32`/`u32` `kind`, the declaration permutes, `s8` `part`/`end`,
 *     a second counter or a `while` loop.
 * SHAPES. The debris pieces are indexed (`work->pieces_0x04[i]`), not walked by pointer.  `eft_em_tint_create` takes
 *   its float before the colour and frame count.  The case-0 hide branch counts with `end` and stops at `part`
 *   (the show branch the other way round).
 */
#include "types.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/push_eft_effect_heap_num.h"
#include "ef/effect.h"
#include "ef/fn_800CDB2C.h"
#include "hud/layout.h"
#include "stage/stg_w.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/printf.h"
#include "lobby/fn_802FA9A0.h"
#include "lobby/fn_8030121C.h"
#include "lobby/LbStr.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_cursor_prompt_draw.h"
#include "lobby/lb_button_prompt_draw.h"
#include "lobby/lb_text_center_print.h"
#include "lobby/lb_item_cell_draw_layout1.h"
#include "lobby/lb_item_cell_draw_layout8.h"
#include "lobby/lb_cancel_prompt_draw.h"
#include "lobby/lb_item_box_count_draw.h"
#include "lobby/lb_tr_flag_get.h"
#include "lobby/lb_tr_flag_data.h"
#include "ef/eft052.h"
#include "ef/eft052_item_get_open.h"
#include "menu/menu_message.h"
#include "menu/put_menu_cursor.h"
#include "menu/menu_item_row_draw.h"
#include "menu/get_pop_dat_ptr.h"
#include "hud/cockpit.h"
#include "hud/put_button_guide_c.h"
#include "hud/put_button_guide_c_idx.h"
#include "hud/draw_sprite_blink_idx.h"
#include "sound/fn_800D7F54.h"
#include "sound/set_zmode__FbUcb.h"
#include "camera/camera.h"
#include "font/flfnt.h"
#include "fn_80047398.h"
#include "menu/menu_item.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em_common.h"
#include "enemy/fn_8012BDF4.h"
#include "stage/get_now_mapno.h"
#include "mh3_pad/vec3.h"
#include "nw4r/math.h"
#include "ef/mtx34_trans_add.h"
#include "ef/mtx34_trans_get.h"
#include "ef/eft001.h"
#include "rotLocalMatX.h"
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h"
#include "lobby/lb_server_sel_trans.h"
#include "sound/se.h"
#include "g3d/g3d_calcworld.h"
#include "sound/se_req_pos_impact.h"
#include "ef/eft041_model_create.h"
#include "ef/eft041_move.h"
#include "ef/eft041_end.h"
#include "ef/eft041_kill.h"
#include "stage/get_stg_eft_col.h"
#include "setScaleMat34W.h"
#include "enemy/fn_8011D448.h"
#include "gx.h"
#include "ef/eft_em_tint.h"

/* The effect-41/42 spawners' pooled work block (`_EFT::work_0x38`) as this band views it.  The count
 * and the two parallel arrays the machines walk: the `effect` slots at +0x04 (stride 0x8, handed to
 * `push_eft_effect_heap_num` / `eft_res_models_release` as a block) and the four g3d part handles at +0x0C
 * (stride 0x10).  size: 0x20 (approximate: the pooled block is larger). */
typedef struct Eft8030EffectSlot {
    /* +0x000 */ nw4r::ef::Effect* effect;
    /* +0x004 */ u8 pad_0x004[0x4];
} Eft8030EffectSlot; /* size: 0x8 */

typedef struct Eft8030ModelSlot {
    /* +0x000 */ void* parts_0x000[4];
} Eft8030ModelSlot; /* size: 0x10 */

typedef struct Eft8030Work {
    /* +0x000 */ s32 count;
    /* +0x004 */ Eft8030EffectSlot slots_0x004[1];
    /* +0x00C */ Eft8030ModelSlot models_0x00C[1];
    /* +0x01C */ u8* source_0x01C;
} Eft8030Work; /* size: 0x20 (approximate: the pooled block is larger) */

/* One debris piece (`EftTintWork::pieces_0x04`): its fade state and alpha, its model, its velocity and size.
 * size: 0x18 */
typedef struct EftTintPiece {
    /* +0x00 */ u8 fade_0x00;     /* 1 once the record's timer is under 10 */
    /* +0x01 */ u8 alpha_0x01;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ VEC3 vel_0x08;
    /* +0x14 */ f32 size_0x14;
} EftTintPiece; /* size: 0x18 */

/* The debris effect's pooled work block (`_EFT::work_0x38`, the 0xE8 bytes `eft_res_slot_get` hands out): the
 * pieces, the effect, the joint, the fall step, the scale, the offset and the tint.  size: 0xE8 */
typedef struct EftTintWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ EftTintPiece pieces_0x04[8];
    /* +0xC4 */ s32 effect_count_0xC4;
    /* +0xC8 */ nw4r::ef::Effect* effect_0xC8;
    /* +0xCC */ u32 joint_0xCC;
    /* +0xD0 */ f32 fall_0xD0;
    /* +0xD4 */ f32 scale_0xD4;
    /* +0xD8 */ nw4r::math::VEC3 offset_0xD8;
    /* +0xE4 */ _GXColor color_0xE4;
} EftTintWork; /* size: 0xE8 */

#pragma peephole off
#pragma pool_data off
#pragma optimization_level 4
#pragma fp_contract off


extern "C" {

/* The debris effect's tables (`.sdata`): per variant (`kind` 0 the plain pieces, 1 the splash variant) and per effect
 * id, the launch speed and the fall step of the two piece groups, the drag of each group, the piece sizes, the spin
 * masks, the piece counts, the effect and model ids and the model part ranges. */
/* 0x80792AA0 */ f32 eft_tint_speed_a0[2] = {40.0f, 40.0f};
/* 0x80792AA8 */ f32 eft_tint_speed_b0[2] = {80.0f, 80.0f};
/* 0x80792AB0 */ f32 eft_tint_fall_a0[2] = {3.0f, 5.0f};
/* 0x80792AB8 */ f32 eft_tint_fall_b0[2] = {4.0f, 6.0f};
/* 0x80792AC0 */ f32 eft_tint_speed_a1[2] = {30.0f, 0.0f};
/* 0x80792AC8 */ f32 eft_tint_speed_b1[2] = {40.0f, 0.0f};
/* 0x80792AD0 */ f32 eft_tint_fall_a1[2] = {0.5f, 0.0f};
/* 0x80792AD8 */ f32 eft_tint_fall_b1[2] = {0.5f, 1.0f};
/* 0x80792AE0 */ f32 eft_tint_drag_a0[2] = {0.9f, 0.93f};
/* 0x80792AE8 */ f32 eft_tint_drag_b0[2] = {0.86f, 0.97f};
/* 0x80792AF0 */ f32 eft_tint_drag_a1[2] = {0.9f, 0.0f};
/* 0x80792AF8 */ f32 eft_tint_drag_b1[2] = {0.9f, 0.0f};
/* 0x80792B00 */ f32* eft_tint_drag_a[2] = {eft_tint_drag_a0, eft_tint_drag_a1};
/* 0x80792B08 */ f32* eft_tint_drag_b[2] = {eft_tint_drag_b0, eft_tint_drag_b1};
/* 0x80792B10 */ f32* eft_tint_speed_a[2] = {eft_tint_speed_a0, eft_tint_speed_a1};
/* 0x80792B18 */ f32* eft_tint_speed_b[2] = {eft_tint_speed_b0, eft_tint_speed_b1};
/* 0x80792B20 */ f32* eft_tint_fall_a[2] = {eft_tint_fall_a0, eft_tint_fall_a1};
/* 0x80792B28 */ f32* eft_tint_fall_b[2] = {eft_tint_fall_b0, eft_tint_fall_b1};
/* 0x80792B30 */ f32 eft_tint_sizes[2] = {1.2f, 0.5f};
/* 0x80792B38 */ u16 eft_tint_spin_mask0[2] = {0x1FFF, 0x0FFF};
/* 0x80792B3C */ u16 eft_tint_spin_mask1[2] = {0x0FFF, 0x0000};
/* 0x80792B40 */ u16* eft_tint_spin_masks[2] = {eft_tint_spin_mask0, eft_tint_spin_mask1};
/* 0x80792B48 */ u8 eft_tint_piece_counts[4] = {8, 6, 0, 0};
/* 0x80792B4C */ u8 eft_tint_group_a_counts[4] = {4, 3, 0, 0};
/* 0x80792B50 */ u16 eft_tint_res_ids[4] = {0x0171, 0x01E8, 0x0616, 0x0000};
/* 0x80792B58 */ u8 eft_tint_res_groups[4] = {0x2B, 0x33, 0x71, 0x00};
/* 0x80792B5C */ u8 eft_tint_model_ids[4] = {0x36, 0x3C, 0x36, 0x00};
/* 0x80792B60 */ s8 eft_tint_part_first[4] = {1, 1, 1, 0};
/* 0x80792B64 */ s8 eft_tint_part_end[4] = {3, 5, 3, 0};
/* 0x80792B68 */ s8 eft_tint_part_mid[8] = {2, 3, 2, 0, 0, 0, 0, 0};

/* The unit's own entry points, declared here because the callers precede the definitions. */
_EFT* eft_em_tint_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                         s32 life);
void eft_em_tint_release(_EFT* self);
void eft_em_tint_dispatch(_EFT* self);
void eft_em_tint_model_create(_EFT* self);
void eft_em_tint_move(_EFT* self);
void eft_em_tint_end(_EFT* self);
void eft_em_tint_kill(_EFT* self);
void eft041_release(_EFT* self);
void eft041_dispatch(_EFT* self);

/* `stage/fn_802B2AA0.h` declares it, but its `eft_vec_ctor` clashes with `ef.h`'s, so the declaration is the same
 * spelling here. */
u8* stage_area_word_get(u8 kind);

/* 0x80305924 (0xC): spawns the debris effect `id` at joint `joint` of `self`, tinted `color`, for 10 frames. */
void eft_em_spawn_tinted(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color) {
    eft_em_tint_create(self, id, joint, pos, scale, color, 10);
}

/* 0x80305930 (0x8): the same spawn with the caller's frame count. */
void eft_em_spawn_tinted_ex(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                            u32 life) {
    eft_em_tint_create(self, id, joint, pos, scale, color, life);
}

/* 0x80305938 (0x21C): pools a debris record for `self`: one model per piece, each sized from the id's table with a
 * random step, then the joint, offset, scale and colour. */
_EFT* eft_em_tint_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                         s32 life) {
    _EFT* rec;
    EftTintWork* work;
    s32 i;

    if (self->area_no != get_now_areano()) {
        return NULL;
    }
    rec = (_EFT*)eft_res_slot_get(0xE8);
    if (rec == NULL) {
        return NULL;
    }
    work = (EftTintWork*)rec->work_0x38;
    work->effect_count_0xC4 = 1;
    work->count_0x00 = eft_tint_piece_counts[id];
    rec->type_0x02 = id;
    rec->release_0x40 = eft_em_tint_release;
    rec->dispatch_0x34 = eft_em_tint_dispatch;
    for (i = 0; i < work->count_0x00; i++) {
        work->pieces_0x04[i].model_0x04 = (MHchar*)eft_res_model_get();
        if (work->pieces_0x04[i].model_0x04 == NULL) {
            eft_res_slot_release(rec);
            return NULL;
        }
        work->pieces_0x04[i].size_0x14 = scale * (eft_tint_sizes[id] + 0.1f * (f32)(ran_suu(0) & 3));
        work->pieces_0x04[i].alpha_0x01 = 0xFF;
    }
    rec->field_0x03 = 0x28;
    rec->source_0x30 = self;
    rec->timer_0x0C = 40;
    rec->rot_0x24.x = 0;
    rec->rot_0x24.y = 0;
    rec->area_0x44 = self->area_no;
    rec->field_0x10 = life;
    copyVec3(&work->offset_0xD8, pos);
    work->joint_0xCC = joint;
    work->scale_0xD4 = scale * get_em_scale(self);
    work->color_0xE4.r = (color & 0xFF000000) >> 24;
    work->color_0xE4.g = (color & 0xFF0000) >> 16;
    work->color_0xE4.b = (color & 0xFF00) >> 8;
    work->color_0xE4.a = color;
    eft_state_flags_set(rec, 0, 0);
    return rec;
}

/* 0x80305B54 (0x78): releases the debris record's piece models and its effect. */
void eft_em_tint_release(_EFT* self) {
    s32 i;
    EftTintWork* work = (EftTintWork*)self->work_0x38;

    for (i = 0; i < work->count_0x00; i++) {
        eft_res_models_release(&work->pieces_0x04[i].model_0x04, 1);
    }
    push_eft_effect_heap_num(&work->effect_0xC8, work->effect_count_0xC4);
    work->count_0x00 = 0;
}

/* 0x80305BCC (0x3C): steps the debris record by state: create, move, end, kill. */
void eft_em_tint_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft_em_tint_model_create(self);
        break;
    case 1:
        eft_em_tint_move(self);
        break;
    case 2:
        eft_em_tint_end(self);
        break;
    case 3:
        eft_em_tint_kill(self);
        break;
    }
}

/* 0x80305C08 (0x674): places the debris on its joint, creates its effect (the splash variant under water) and its
 * piece models, launches each piece in a random direction and picks the pieces' visible parts. */
void eft_em_tint_model_create(_EFT* self) {
    nw4r::math::VEC3 vel;
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    EftTintWork* work = (EftTintWork*)self->work_0x38;
    struct _ENEMY_WORK* enemy = (struct _ENEMY_WORK*)self->source_0x30;
    s32 i;
    u8 kind;
    u8 pick;
    s32 end;
    u8 type;
    s32 part;

    VEC3_ctor(&vel);
    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    copyVec3(&ofs, &work->offset_0xD8);
    get_joint_wmat_em(enemy, work->joint_0xCC, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    mulVecMat(&ofs, &mtx);
    addVec3To(&self->pos_0x18, &ofs);
    type = self->type_0x02;
    if (type - 1U > 1) {
        if (type == 0) {
            if (enemy->field_0x1E2 != 2 || !(self->pos_0x18.y < enemy->field_0x210)) {
                work->effect_0xC8 =
                    res_eft_create(eft_tint_res_ids[type], eft_tint_res_groups[type], 0);
                if (work->effect_0xC8 == NULL) {
                    eft_em_tint_kill(self);
                    return;
                }
                kind = 0;
            } else {
                work->effect_0xC8 = res_eft_create(0x1F5, 0x2B, 0);
                if (work->effect_0xC8 == NULL) {
                    eft_em_tint_kill(self);
                    return;
                }
                kind = 1;
            }
        }
    } else {
        work->effect_0xC8 = res_eft_create(eft_tint_res_ids[type], eft_tint_res_groups[type], 0);
        if (work->effect_0xC8 == NULL) {
            eft_em_tint_kill(self);
            return;
        }
        kind = 0;
    }
    change_paramscale_eff(work->effect_0xC8, work->scale_0xD4);
    SetRootMtxTrans(work->effect_0xC8, &self->pos_0x18);
    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->pieces_0x04[i].model_0x04, eft_tint_model_ids[self->type_0x02], 0x18) == NULL) {
            eft_em_tint_kill(self);
            return;
        }
        work->pieces_0x04[i].model_0x04->ready = 1;
        copyVec3((nw4r::math::VEC3*)&work->pieces_0x04[i].model_0x04->pos_0x04, &self->pos_0x18);
        work->pieces_0x04[i].model_0x04->scale_0x1C.x = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->scale_0x1C.y = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->scale_0x1C.z = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->field_0x28 = ran_suu(0);
        work->pieces_0x04[i].model_0x04->field_0x2C = ran_suu(0);
        work->pieces_0x04[i].model_0x04->field_0x30 = ran_suu(0);
        if (i < eft_tint_group_a_counts[self->type_0x02]) {
            setVector3(&vel, 0.0f, 0.0f, eft_tint_speed_a[kind][self->type_0x02]);
            work->fall_0xD0 = eft_tint_fall_a[kind][self->type_0x02];
        } else {
            setVector3(&vel, 0.0f, 0.0f, eft_tint_speed_b[kind][self->type_0x02]);
            work->fall_0xD0 = eft_tint_fall_b[kind][self->type_0x02];
        }
        rotVecX(&vel, 0xC000 - (u16)(0.5f + 65536.0f * (f32)(u32)(ran_suu(0) % 60) / 360.0f));
        rotVecY(&vel, ran_suu(0));
        copyVec3((nw4r::math::VEC3*)&work->pieces_0x04[i].vel_0x08, &vel);
    }
    switch (self->type_0x02) {
    case 0:
        for (i = 0; i < work->count_0x00; i++) {
            if (ran_suu(0) & 1) {
                for (part = eft_tint_part_first[self->type_0x02]; part < (end = eft_tint_part_mid[self->type_0x02]);
                     part++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                }
                for (; end < eft_tint_part_end[self->type_0x02]; end++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(end, 0);
                }
            } else {
                for (end = eft_tint_part_first[self->type_0x02]; end < (part = eft_tint_part_mid[self->type_0x02]);
                     end++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(end, 0);
                }
                for (; part < eft_tint_part_end[self->type_0x02]; part++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                }
            }
        }
        break;
    case 1:
        for (i = 0; i < 2; i++) {
            pick = (ran_suu(0) & 1) + 1;
            for (part = eft_tint_part_first[self->type_0x02]; part < eft_tint_part_end[self->type_0x02]; part++) {
                if (part == pick) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                } else {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 0);
                }
            }
        }
        for (i = 2; i < work->count_0x00; i++) {
            pick = (ran_suu(0) & 1) + 3;
            for (part = eft_tint_part_first[self->type_0x02]; part < eft_tint_part_end[self->type_0x02]; part++) {
                if (part == pick) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                } else {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 0);
                }
            }
        }
        if (self->area_0x44 == get_now_areano()) {
            se_req_pos_ps(enemy->se_0xB14, 0x25, 2, &self->pos_0x18);
        }
        break;
    }
    self->demo_flag_0x08 = kind;
    self->flag_0x01 = 1;
    eft_em_tint_move(self);
}

/* 0x8030627C (0x294): after the record's delay, moves each piece (drag, fall, spin), fades it out over the last 10
 * frames, and runs and tints the effect; the record ends when its timer runs out. */
void eft_em_tint_move(_EFT* self) {
    _GXColor col;
    s32 i;
    u16* mask;
    EftTintWork* work = (EftTintWork*)self->work_0x38;

    if (--self->field_0x10 < 0) {
        if (--self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        col.r = work->color_0xE4.r;
        col.g = work->color_0xE4.g;
        col.b = work->color_0xE4.b;
        mask = eft_tint_spin_masks[self->demo_flag_0x08];
        for (i = 0; i < work->count_0x00; i++) {
            if (i < eft_tint_group_a_counts[self->type_0x02]) {
                vec3_scale_in_place((VEC3*)&work->pieces_0x04[i].vel_0x08, eft_tint_drag_a[self->demo_flag_0x08][self->type_0x02]);
            } else {
                vec3_scale_in_place((VEC3*)&work->pieces_0x04[i].vel_0x08, eft_tint_drag_b[self->demo_flag_0x08][self->type_0x02]);
            }
            work->pieces_0x04[i].vel_0x08.y -= work->fall_0xD0;
            addVec3To(&work->pieces_0x04[i].model_0x04->pos_0x04, &work->pieces_0x04[i].vel_0x08);
            work->pieces_0x04[i].model_0x04->field_0x28 += mask[self->type_0x02] & ran_suu(0);
            work->pieces_0x04[i].model_0x04->field_0x2C += mask[self->type_0x02] & ran_suu(0);
            work->pieces_0x04[i].model_0x04->field_0x30 += mask[self->type_0x02] & ran_suu(0);
            switch (work->pieces_0x04[i].fade_0x00) {
            case 0:
                if (self->timer_0x0C < 10) {
                    work->pieces_0x04[i].fade_0x00 = 1;
                }
                break;
            case 1:
                if (self->timer_0x0C > 0) {
                    work->pieces_0x04[i].alpha_0x01 -= 25;
                } else {
                    work->pieces_0x04[i].alpha_0x01 = 0;
                }
                break;
            }
            col.a = work->pieces_0x04[i].alpha_0x01;
            work->pieces_0x04[i].model_0x04->setTevKColor(0, (_GXTevKColorID)3, &col);
            work->pieces_0x04[i].model_0x04->move(0);
            eft_res_models_spawn(self, (void**)&work->pieces_0x04[i].model_0x04, 2, 1, NULL);
        }
        effect_move(work->effect_0xC8);
        change_color_eff(work->effect_0xC8, &self->pos_0x18, work->color_0xE4);
        eft_res_models_spawn(self, (void**)&work->effect_0xC8, 1, 1, NULL);
    }
}

/* 0x80306510 (0x10): the debris end step - advances to the kill step. */
void eft_em_tint_end(_EFT* self) {
    self->state_0x05++;
}

/* 0x80306520 (0x4): the debris kill step - returns the record to the pool. */
void eft_em_tint_kill(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x80306524 - builds the effect-41 spawner: pools a 0x20-byte effect record, installs the
 * `eft041_dispatch` dispatcher / `eft041_release` releaser, fills its one model slot and arms it. */
void lb_area_fx_set(void) {
    u8 area = get_now_areano();
    u8* res;
    s32 i;
    _EFT* rec;
    Eft8030Work* w;

    res = stage_area_word_get(area);
    if (res == NULL) {
        return;
    }
    rec = (_EFT*)eft_res_slot_get(0x20);
    if (rec == NULL) {
        return;
    }
    rec->dispatch_0x34 = eft041_dispatch;
    rec->release_0x40 = eft041_release;
    w = (Eft8030Work*)rec->work_0x38;
    w->source_0x01C = res;
    w->count = 1;
    for (i = 0; i < w->count; i++) {
        w->slots_0x004[i].effect = (nw4r::ef::Effect*)eft_res_model_get();
        if (w->slots_0x004[i].effect == NULL) {
            eft_res_slot_release(rec);
            return;
        }
    }
    memset(w->models_0x00C, 0, 0x10);
    rec->field_0x03 = 0x29;
    eft_state_flags_set(rec, 8, 0);
    rec->area_0x44 = area;
}

/* 0x80306614 - releases the effect-41 spawner's models and pooled handles. */
void eft041_release(_EFT* self) {
    s32 i;
    s32 j;
    Eft8030Work* w = (Eft8030Work*)self->work_0x38;

    for (i = 0; i < w->count; i++) {
        for (j = 3; j >= 0; j--) {
            if (w->models_0x00C[i].parts_0x000[j] != NULL) {
                push_g3d_wk((_g3d_work*)w->models_0x00C[i].parts_0x000[j]);
            }
        }
        if (w->slots_0x004[i].effect != NULL) {
            eft_res_models_release(&w->slots_0x004[i], 1);
        }
    }
}

/* 0x803066B4 (0x3C): steps the area effect by state: create, move, end, kill. */
void eft041_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft041_model_create(self);
        break;
    case 1:
        eft041_move(self);
        break;
    case 2:
        eft041_end(self);
        break;
    case 3:
        eft041_kill(self);
        break;
    }
}

} /* extern "C" */
