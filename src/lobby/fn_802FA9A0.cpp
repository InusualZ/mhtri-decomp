/* lobby/fn_802FA9A0.cpp - the screen-filter effect's steps, the lobby progress/unlock predicates, the torch effect,
 *   the em023 program and the whale-event trader's barter screen.
 * RANGE. .text 0x802FA9A0-0x8030121C (120 functions); .ctors 0x8056F394-0x8056F398 (MWCC's `__sinit` of
 *   `em023_se_ofs_a`/`_b`), .rodata 0x805707C0-0x80570840, .data 0x805D8B00-0x805DAFE8, .bss 0x806BE0D8-0x806BE108,
 *   .sdata 0x80792948-0x80792A58, .sdata2 0x8079AC20-0x8079AD98, extab, extabindex.  Function families in address
 *   order: eft036 (0x802FA9A0-0x802FAFC8, the type-0x24 camera filter), the unlock predicates (0x802FAFC8-0x802FBA94),
 *   eft038 (0x802FBA94-0x802FBE44, the type-0x26 lobby torch), em023 (0x802FBE44-0x802FF234, `em023_prog_tbl` at .data
 *   0x805D9040, its `__sinit` at 0x802FF180), the whale event and its barter screen (lobby screen 11,
 *   0x802FF234-0x8030121C, `lb_barter_*`).  The range is four or five retail TUs (seam requests lobby-802fa9a0#1-#4):
 *   the eft036 pool repeats values of `enemy/em_sub_state_prog.cpp`'s (whose 0x802FA804-0x802FA9A0 tail is eft036's
 *   spawner, release and dispatcher), eft038's pool repeats 1.0, em023's TU ends with its `__sinit`, and each family
 *   opens its own `.sdata2` run (0x8079AC20, 0x8079AC40, 0x8079AC48, 0x8079AD88).
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off`: retail keeps `slwi` after `clrlwi` and `srwi` + `clrlwi`
 *   apart where the pass folds them into one `rlwinm` (`lb_unlock_cond_tbl0_ck`, `lb_progress_flag_b8_ck`).
 * NAMES. No `__FILE__` string and no runtime-dump name covers the range (`LbCheckKujiraEvent__Fv` is the one real
 *   name), so the file keeps its registered stem.  Every other name is a GUESS from the body, its callers and the
 *   effect type it seeds: eft036_* (type 0x24), eft038_* (type 0x26), lb_unlock_cond_*, lb_progress_unlock,
 *   lb_npc_appear_ck, lb_unlock_rank_get, lb_world_mode_get, lb_world_upgrade_get, lb_event_*, lb_pig_dress_refresh,
 *   lb_kujira_* and kujira_event_over_ck (the whale event: the trader's visit), lb_barter_* (its screen), em023_* (slot
 *   names after `Em015ProgTbl`'s scheme, the action groups by motion and purpose); the data rows eft036_spheres_*,
 *   eft038_torch_pos, lb_unlock_cond_tbl0/1, em023_turn_seq_a/b, em023_se_seq_*, em023_se_alt_seq_*, em023_route_*,
 *   em023_demo5_keys_a..f, em023_se_ofs_a/b, lb_barter_* tables, lb_kujira_stay_ranges.
 * RESIDUALS. Every row is written; 13 rows are short of 100:
 *   - em023_stage_actor_busy_ck: retail narrows the area parameter in place in r31 before the loop and colours max/i
 *     r29/r28; every parameter/local order tried leaves the narrowing in the loop;
 *   - lb_barter_entry_flags: retail keeps `index * 12` live across the calls, ours keeps `index` and re-multiplies;
 *   - lb_barter_step: the tab switch is lowered as an unsigned range test then a signed `cmpwi 2` in retail (ours a
 *     signed binary search), and `3 - (ok == 0)` is `neg`+`addi` there (ours `subfic`);
 *   - lb_barter_cart_slot_find: retail returns a first-loop hit through the function's single exit (ours its own
 *     `mr r3`); the shape needs a jump rule 8 forbids;
 *   - lb_kujira_day_step, lb_barter_stock_add, lb_barter_page_fill, lb_barter_price_scale, lb_barter_refs_kind_build,
 *     lb_barter_settle, lb_barter_page_fill_kind, lb_barter_open: register colouring or operand order only;
 *   - eft036_move: retail tests kind 2 with a signed `cmpwi` after an unsigned range test (ours `cmplwi`, the switch is
 *     on `(u32)kind`), the player record pointer is r23 where ours is r24, and the three `get_camera_*` return
 *     temporaries sit above `pos`/`offset` in retail's frame.
 *   MWCC emits the `__sinit` at the end of `.text` where retail has it at 0x802FF180 (the end of the em023 TU).
 *   flipcheck: `.rodata`/`.data`/`.sdata`/`.sdata2` claimed, not emitted (reached through `extern` declarations: one
 *   source cannot lay the TUs' pools out in retail order); `.text` order differs at the `__sinit`.
 * SHAPES. The six id-table predicates take a `u32` and narrow it in a temporary.  The eft036 steps keep the
 *   dispatcher's `_ENEMY_WORK*` parameter and view it as the `_EFT` it is.  `light_point_scale_set` takes its float
 *   before the flag byte, and the two `lb_npc_talk_ok_*` results are compared unsigned.  In em023 a one-value test
 *   retail compares signed (`cmpwi`) is a one-case `switch`, the degree-to-angle macro converts through `s32`, the
 *   turn-away heading is adjusted in place (`+=`/`-=`), the 3-argument `eft_slot_marks_set_sync` call and the float
 *   argument of `em_ground_ck` go through casts of the owner's declarations.  In the barter screen the loops index
 *   their tables (`a[i]`, not walking pointers), loop counters are `s16` where retail unrolls by two and `s32` where it
 *   unrolls fully, row indices into the u16 reference table are `u16`, `toggle_word_step`'s last argument is a `u16`,
 *   and the goods table is picked with a conditional expression.
 */
#include "types.h"
#include "lobby/fn_802FA9A0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_res.h"
#include "enemy/fn_8012EC74.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/printf.h"
#include "lobby/LbStr.h"
#include "menu/menu_message.h"
#include "ef/eft052.h"
#include "enemy/em020_ai.h"
#include "stage/stg_w.h"
#include "mh3_pad/lb_param_w.h"
#include "lobby/lb_ship_open.h"
#include "lobby/lb_entry_changed_clr.h"
#include "lobby/lb_npc_dress_refresh_all.h"
#include "lobby/lb_world_submode_get.h"
#include "lobby/fn_801E7530.h"
#include "userdata_online_first_set.h"
#include "pl.h"
#include "ef.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad.h"
#include "nw4r/math.h"
#include "camera/camera.h"
#include "camera/camera_angle_x_get.h"
#include "stage/get_now_mapno.h"
#include "Pl/pl_hit_sphere.h"
#include "sound/mhchar_mat_alpha_set.h"
#include "ef/get_move_work_adrs.h"
#include "ef/effect.h"
#include "ef/ef_effect.h"
#include "ef/push_eft_effect_heap_num.h"
#include "ef/res_eft_create_ex.h"
#include "stage/stage_torch_unlit_ck.h"
#include "stage/stage_area_objects_refresh.h"
#include "sound/se_torch_slot_req.h"
#include "sound/fn_800D7F54.h"
#include "light/light.h"
#include "light/light_point_scale_set.h"
#include "lobby/lb_server_sel_trans.h"
#include "enemy/em_common.h"
#include "enemy/fn_8012BDF4.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/em_se_record.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/em_angle_approach.h"
#include "enemy/em_hit_arm_set.h"
#include "enemy/em_hit_by_set.h"
#include "enemy/em_hit_window_set_default.h"
#include "enemy/em_mot_end_ck.h"
#include "enemy/em_mot_speed_set.h"
#include "enemy/em_motion_kind_set.h"
#include "enemy/em_move_target_set.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"
#include "enemy/fn_8011D448.h"
#include "enemy/em015_act13_early_ck.h"
#include "enemy/em018_act13_early_ck.h"
#include "ef/eft_slot.h"
#include "ef/eft_slot_shape_hit_ck.h"
#include "lobby/fn_8030121C.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_panel_close.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/weight_table_index_u8.h"
#include "lobby/lb_kujira_gift_items.h"
#include "ef/system_core.h"
#include "ef/eft052_item_get_open.h"
#include "userdata_item.h"

#pragma peephole off

extern "C" {

/* One camera-hide sphere of the screen-filter effect's tables: inside it the filter fades out; a negative radius ends
 * a table.  size: 0x10 */
struct Eft036Sphere {
    /* +0x00 */ VEC3 center_0x00;
    /* +0x0C */ f32 radius_0x0C;
};

/* The work block of the camera-attached screen-filter effect (type 0x24): one pooled model, its UV-animated model
 * object, the handle pair the create files, the kind and the current alpha.  size: 0x1C (the size
 * `eft_res_slot_get(0x1C)` pools in its spawner) */
struct Eft036Work {
    /* +0x00 */ u32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[1];
    /* +0x08 */ EftUvModelObject* uv_0x08;
    /* +0x0C */ struct _g3d_work* works_0x0C[1][2];
    /* +0x14 */ u8 kind_0x14;        /* 0 water surface, 1 lobby area filter, 2 the third map filter */
    /* +0x15 */ u8 pad_0x15[3];
    /* +0x18 */ f32 alpha_0x18;      /* the filter's alpha, 0..1 */
};

/* The camera-hide sphere tables of map 3 (areas 0, 8, 9, 10) and map 21 (area 1). */
extern Eft036Sphere eft036_spheres_m03_a00[2];
extern Eft036Sphere eft036_spheres_m03_a08[2];
extern Eft036Sphere eft036_spheres_m03_a09[2];
extern Eft036Sphere eft036_spheres_m03_a10[2];
extern Eft036Sphere eft036_spheres_m21_a01[2];

/* Creates the filter's UV model for every pooled model, seeds the effect and the models' scale and the model's
 * scalar per kind, then runs the first move. */
void eft036_model_create(struct _ENEMY_WORK* work) {
    _EFT* self = (_EFT*)work;
    Eft036Work* w = (Eft036Work*)self->work_0x38;
    u32 i;
    s32 id;
    f32 scalar;
    f32 scale;

    for (i = 0; i < w->count_0x00; i++) {
        switch (w->kind_0x14) {
        case 0:
            id = 51;
            break;
        case 1:
            id = 64;
            break;
        case 2:
            id = (get_now_mapno() == 6) ? 65 : 66;
            break;
        }
        w->uv_0x08 = (EftUvModelObject*)res_eft_UV_model_create(w->models_0x04[i], id, 332, 0, w->works_0x0C[i], 1, 1);
        if (w->uv_0x08 == NULL) {
            eft036_release(work);
            return;
        }
    }
    self->state_0x05++;
    self->field_0x06 = 0;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    self->area_0x44 = get_now_areano();
    scale = 1.0f;
    w->alpha_0x18 = scale;
    switch (w->kind_0x14) {
    case 0:
        scalar = 2.0f;
        break;
    case 1:
        scalar = 20.0f;
        scale = 1.5f;
        break;
    case 2:
        scalar = 2.0f;
        break;
    }
    for (i = 0; i < w->count_0x00; i++) {
        setVector3(&w->models_0x04[i]->scale_0x1C, scale, scale, scale);
        w->models_0x04[i]->field_0x28 = 0;
        w->models_0x04[i]->field_0x2C = 0;
        w->models_0x04[i]->field_0x30 = 0;
        w->models_0x04[i]->ready = 3;
        w->uv_0x08->set_scalar(scalar);
    }
    eft036_move(work);
}

/* Places the filter model in front of the camera each frame and fades it: kind 0 shows it only under water, kinds
 * 1 and 2 fade it out inside the area's camera-hide spheres or past the third map's line. */
void eft036_move(struct _ENEMY_WORK* work) {
    _EFT* self = (_EFT*)work;
    Eft036Work* w = (Eft036Work*)self->work_0x38;
    VEC3 cam_pos;
    f32 dist;
    f32 alpha_max;
    u8 hide;
    VEC3 dir;
    VEC3 center;
    Eft036Sphere* sphere;
    EmAreaWork* players;
    EmAreaWork* player;
    u32 i;

    VEC3_ctor(&cam_pos);
    dist = 0.0f;
    alpha_max = 1.0f;
    hide = 0;
    VEC3_ctor(&dir);
    VEC3_ctor(&center);
    sphere = NULL;
    players = (EmAreaWork*)get_move_work_adrs(2);
    player = players + (s8)my_player_no();
    if (w->kind_0x14 != 1 || (lb_event_gate2_ck() != 1 && LbCheckKujiraEvent() != 0)) {
        switch ((u32)w->kind_0x14) {
        case 0:
        case 1:
            copyVec3(&cam_pos, &get_camera_pos());
            switch (get_now_mapno()) {
            case 3:
                switch (self->area_0x44) {
                case 0:
                    sphere = eft036_spheres_m03_a00;
                    break;
                case 8:
                    sphere = eft036_spheres_m03_a08;
                    break;
                case 9:
                    sphere = eft036_spheres_m03_a09;
                    break;
                case 10:
                    sphere = eft036_spheres_m03_a10;
                    break;
                }
                break;
            case 21:
                if (self->area_0x44 == 1) {
                    sphere = eft036_spheres_m21_a01;
                }
                break;
            }
            if (sphere != NULL) {
                for (; sphere->radius_0x0C > 0.0f; sphere++) {
                    vec_to_mh_vec3(&center, (struct Vec*)&sphere->center_0x00);
                    if (hit_point_sphr(&cam_pos, &center, sphere->radius_0x0C) == 1) {
                        hide = 1;
                        break;
                    }
                }
            }
            break;
        case 2:
            hide = !(player->vec_0x3C.x <= -2100.0f);
            break;
        }
        if (self->field_0x06 == 0) {
            if (hide) {
                w->alpha_0x18 = 0.0f;
            }
            self->field_0x06++;
        }
        if (hide) {
            w->alpha_0x18 -= 0.05f;
            if (w->alpha_0x18 < 0.0f) {
                w->alpha_0x18 = 0.0f;
            }
        } else {
            w->alpha_0x18 += 0.05f;
            if (w->alpha_0x18 > alpha_max) {
                w->alpha_0x18 = alpha_max;
            }
        }
        for (i = 0; i < w->count_0x00; i++) {
            VEC3 pos;
            VEC3 offset;

            copyVec3(&dir, &get_camera_direction());
            vec3_normalize_into(&dir, &dir);
            if (w->kind_0x14 != 0) {
                dist = 20.0f;
            }
            vec3_scale(&offset, &dir, dist);
            addVec3(&pos, &get_camera_pos(), &offset);
            copyVec3(&w->models_0x04[i]->pos_0x04, &pos);
            w->models_0x04[i]->field_0x28 = camera_angle_x_get();
            w->models_0x04[i]->field_0x2C = camera_angle_y_get();
            w->models_0x04[i]->field_0x30 = 0;
            w->models_0x04[i]->move(0);
            w->uv_0x08->update();
            switch (w->kind_0x14) {
            case 0:
                if (stage_water_enabled_ck() != 0 && camera_underwater_ck() != 0) {
                    hide = 1;
                }
                if (hide) {
                    self->flag_0x01 = 0;
                } else {
                    self->flag_0x01 = 1;
                }
                break;
            case 1:
                if (sphere != NULL) {
                    mhchar_mat_alpha_set(w->models_0x04[i], 4, (u8)(255.0f * w->alpha_0x18), 0);
                }
                break;
            case 2:
                mhchar_mat_alpha_set(w->models_0x04[i], 4, (u8)(255.0f * w->alpha_0x18), 0);
                break;
            }
            eft_res_models_spawn(self, (void**)&w->models_0x04[i], 2, 1, 0);
        }
    }
}

/* Advances the effect to its end state. */
void eft036_end(struct _ENEMY_WORK* work) {
    ((_EFT*)work)->state_0x05++;
}

/* Releases the effect's pooled slot. */
void eft036_release(struct _ENEMY_WORK* work) {
    eft_res_slot_release(work);
}

/* Marks progress entry `id` reached once and applies what it unlocks: the village scene mode, the village/farm/ship
 * levels, the kitchen pair records, the trade ship and the online profile. */
void lb_progress_unlock(s32 id) {
    char text[0x80];
    s8* name;

    if (userdata_progress_flag_ck(id) == 1) {
        return;
    }
    userdata_progress_flag_set(id);
    switch (id) {
    case 0x31:
        lobby_world_block->mode_0x3E01 = 9;
        break;
    case 0x29:
        lobby_world_block->mode_0x3E01 = 5;
        lb_entry_changed_clr();
        break;
    case 0x28:
        lobby_world_block->mode_0x3E01 = 4;
        lb_entry_changed_clr();
        memset(lobby_world_block->kitchen_pairs_0x516C, 0, 0x14);
        break;
    case 0x26:
        if (lobby_world_block->village_rank_0x3E02 < 3) {
            lobby_world_block->village_rank_0x3E02 = 3;
        }
        break;
    case 0x23:
        lb_kujira_bit_set(2);
        memset(lobby_world_block->kitchen_pairs_0x516C, 0, 0x14);
        break;
    case 0x21:
        if (lobby_world_block->village_rank_0x3E02 < 2) {
            lobby_world_block->village_rank_0x3E02 = 2;
        }
        break;
    case 0x1F:
        lobby_world_block->mode_0x3E01 = 3;
        lb_kujira_bit_set(1);
        break;
    case 0x1C:
        lobby_world_block->mode_0x3E01 = 2;
        lb_kujira_work_reset();
        break;
    case 0x24:
        lb_progress_unlock(0x1A);
        break;
    case 0x17:
        lb_ship_open();
        break;
    case 0x67:
        lb_param_w.hunt_option_0x0A = 1;
        break;
    case 0x4D:
        if (lobby_world_block->grade_0x4654[1] < 2) {
            lobby_world_block->grade_0x4654[1] = 2;
        }
        break;
    case 0x4E:
        if (lobby_world_block->grade_0x4654[1] < 3) {
            lobby_world_block->grade_0x4654[1] = 3;
        }
        break;
    case 0x51:
        if (lobby_world_block->grade_0x4654[3] < 1) {
            lobby_world_block->grade_0x4654[3] = 1;
        }
        break;
    case 0x52:
        if (lobby_world_block->grade_0x4654[3] < 2) {
            lobby_world_block->grade_0x4654[3] = 2;
        }
        break;
    case 0x53:
        if (lobby_world_block->grade_0x4654[3] < 3) {
            lobby_world_block->grade_0x4654[3] = 3;
        }
        break;
    case 0x4F:
        if (lobby_world_block->grade_0x4654[0] < 2) {
            lobby_world_block->grade_0x4654[0] = 2;
        }
        break;
    case 0x50:
        if (lobby_world_block->grade_0x4654[0] < 3) {
            lobby_world_block->grade_0x4654[0] = 3;
        }
        break;
    case 0x54:
        if (lobby_world_block->grade_0x4654[2] < 1) {
            lobby_world_block->grade_0x4654[2] = 1;
        }
        break;
    case 0x55:
        if (lobby_world_block->grade_0x4654[2] < 2) {
            lobby_world_block->grade_0x4654[2] = 2;
        }
        break;
    case 0x56:
        if (lobby_world_block->grade_0x4654[2] < 3) {
            lobby_world_block->grade_0x4654[2] = 3;
        }
        break;
    case 0x1A:
        lobby_world_block->mode_0x3E01 = 1;
        if (lobby_world_block->village_rank_0x3E02 < 1) {
            lobby_world_block->village_rank_0x3E02 = 1;
        }
        break;
    case 0x5D:
        if (lobby_world_block->ships_0x4670 < 2) {
            lobby_world_block->ships_0x4670 = 2;
        }
        break;
    case 0x5E:
        if (lobby_world_block->ships_0x4670 < 3) {
            lobby_world_block->ships_0x4670 = 3;
        }
        break;
    case 0x03:
        if (lobby_world_block->level_0x3F3A < 2) {
            lobby_world_block->level_0x3F3A = 2;
        }
        break;
    case 0x00:
        if (lobby_world_block->level_0x3F3A < 1) {
            lobby_world_block->level_0x3F3A = 1;
        }
        break;
    case 0x04:
        if (lobby_world_block->level_0x3F46 < 2) {
            lobby_world_block->level_0x3F46 = 2;
        }
        break;
    case 0x01:
        if (lobby_world_block->level_0x3F46 < 1) {
            lobby_world_block->level_0x3F46 = 1;
        }
        break;
    case 0x05:
        if (lobby_world_block->level_0x3F52 < 2) {
            lobby_world_block->level_0x3F52 = 2;
        }
        break;
    case 0x02:
        if (lobby_world_block->level_0x3F52 < 1) {
            lobby_world_block->level_0x3F52 = 1;
        }
        break;
    case 0x59:
        name = get_player_name_str(3);
        sprintf(text, (const char*)LbStr(0, 0x200), name);
        hud_msg_push(0, text);
        break;
    case 0xA2:
        userdata_online_first_set();
        ((s32 (*)(LbBlockView*))lobby_net_profile_part1_send)(lobby_world_block);
        break;
    case 0x3D:
        memset(lobby_world_block->kitchen_pairs_0x525C, 0, 0x14);
        break;
    case 0x3F:
        memset(lobby_world_block->kitchen_pairs_0x525C, 0, 0x14);
        break;
    }
}

/* Walks the id table for ids 0..27 and reports whether every entry it points at is accepted by the
 * `0x8004D27C` predicate; ids above 27 and table holes answer 0, and the 0xFFFF terminator answers 1. */
u32 lb_unlock_cond_tbl0_ck(u32 raw) {
    u16 id = (u16)raw;
    u16* entry;

    if (id > 27) {
        return 0;
    }
    entry = lb_unlock_cond_tbl0[id];
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (userdata_progress_flag_ck(*entry) == 0) {
            return 0;
        }
        entry++;
    }
    return 1;
}

/* The same walk for the second id table, which covers ids 28..39; the table is indexed from its own
 * base, so the id is rebased by 28. */
u32 lb_unlock_cond_tbl1_ck(u32 raw) {
    u16 id = (u16)raw;
    u16* entry;

    if (id <= 27) {
        return 0;
    }
    entry = lb_unlock_cond_tbl1[id - 28];
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (userdata_progress_flag_ck(*entry) == 0) {
            return 0;
        }
        entry++;
    }
    return 1;
}

/* Dispatches an id to the table that covers it: 0xFFFF (the empty id) and id 0 answer directly, ids
 * 28 and above go to the second table, the rest to the first. */
u32 lb_unlock_cond_ck(u32 raw) {
    u16 id = (u16)raw;

    if (id == 0xFFFF) {
        return 0;
    }
    if (id == 0) {
        return 1;
    }
    if (id >= 28) {
        return lb_unlock_cond_tbl1_ck(id);
    }
    return lb_unlock_cond_tbl0_ck(id);
}

/* Whether `id` and `id + 1` together form the run boundary the caller is looking for: `id` accepted
 * and the next id rejected. */
u32 lb_unlock_cond_last_ck(u32 id) {
    if (lb_unlock_cond_ck((u16)id) == 1 && lb_unlock_cond_ck((u16)(id + 1)) == 0) {
        return 1;
    }
    return 0;
}

/* Returns the first id of the requested table that `lb_unlock_cond_ck` accepts: ids 27 downwards for kind 0
 * and 39 down to 28 for kind 1; any other kind answers 0. */
u16 lb_unlock_cond_top_get(u32 raw) {
    u16 id;

    switch ((u8)raw) {
    case 0:
        for (id = 27; id >= 0; id--) {
            if (lb_unlock_cond_ck(id) != 0) {
                return id;
            }
        }
        break;
    case 1:
        for (id = 39; id >= 28; id--) {
            if (lb_unlock_cond_ck(id) != 0) {
                return id;
            }
        }
        break;
    default:
        return 0;
    }
    return 0;
}

/* The mode byte of the block `lobby_world_block` points at, +0x3E01. */
u8 lb_world_mode_get(void) {
    return lobby_world_block->mode_0x3E01;
}

/* The lobby work block's scene byte at +0x003. */
u8 lb_event_kind_get(void) {
    return lobby_w.field_0x003;
}

/* Whether the NPC of `kind` is placed on the current map: in the online lobby by its kind and the whale event, in
 * the village by the unlock conditions, the farm plots, the ship count and the NPCs' own talk checks. */
u32 lb_npc_appear_ck(u8 kind) {
    u8 slot;

    if (game_ready_ck() == 1) {
        switch (kind) {
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x47:
        case 0x48:
            return 1;
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x2C:
        case 0x35:
        case 0x36:
        case 0x44:
        case 0x45:
        case 0x46:
            if (LbCheckKujiraEvent() == 0) {
                return 1;
            }
            break;
        case 0x24:
        case 0x25:
        case 0x34:
        case 0x37:
            if (LbCheckKujiraEvent() == 1) {
                return 1;
            }
            break;
        case 0x49:
            if ((u32)lb_npc_talk_ok_npc73(kind) == 1) {
                return 1;
            }
            break;
        }
    } else {
        switch (kind) {
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
        case 8:
        case 14:
        case 15:
        case 16:
        case 19:
        case 25:
            return 1;
        case 1:
            if (lb_unlock_cond_ck(12) == 1) {
                return lb_unlock_cond_last_ck(20) != 1;
            }
            break;
        case 4:
            if (lb_unlock_cond_ck(1) == 1) {
                return 1;
            }
            break;
        case 9:
            if (kujira_event_over_ck() == 1) {
                return 1;
            }
            break;
        case 10:
        case 11:
        case 12:
            if (lb_unlock_cond_ck(7) == 1 && userdata_progress_flag_ck(0x1A) != 0) {
                switch (kind) {
                case 10:
                    slot = 0;
                    break;
                case 11:
                    slot = 1;
                    break;
                case 12:
                    slot = 2;
                    break;
                }
                if (slot + 1 > lb_world_submode_get() || lb_part_slot_open_ck(slot) == 1) {
                    return 1;
                }
            }
            break;
        case 13:
            if (lb_unlock_cond_ck(9) == 1) {
                return 1;
            }
            break;
        case 17:
            if (lb_unlock_cond_ck(9) == 1 && kujira_event_over_ck() == 1) {
                return 1;
            }
            break;
        case 20:
            if (lobby_world_block->ships_0x4670 >= 1) {
                return 1;
            }
            break;
        case 21:
            if (lobby_world_block->ships_0x4670 >= 2) {
                return 1;
            }
            break;
        case 22:
            if (lobby_world_block->ships_0x4670 >= 3) {
                return 1;
            }
            break;
        case 23:
            if ((u32)lb_npc_talk_ok_npc23(kind) == 1) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* The village rank the unlock table has reached: 0 once id 6 is accepted, otherwise 1..4 from the top accepted id. */
u8 lb_unlock_rank_get(void) {
    if (lb_unlock_cond_ck(6) == 0) {
        switch (lb_unlock_cond_top_get(0)) {
        case 0:
        case 1:
            return 1;
        case 2:
        case 3:
            return 2;
        case 4:
            return 3;
        case 5:
            return 4;
        }
    }
    return 0;
}

/* Whether the lobby currently stands in id 5's area. */
u8 lb_unlock_cond5_get(void) {
    return lb_unlock_cond_ck(5);
}

/* A per-kind state byte of the block `lobby_world_block` points at, +0x4654. */
u8 lb_world_upgrade_get(u32 raw) {
    return lobby_world_block->states_0x4654[(u8)raw];
}

/* Whether id 22 is accepted and the 0x58 event flag is up. */
u32 lb_unlock_cond22_flag_ck(void) {
    if (lb_unlock_cond_ck(22) == 1 && userdata_progress_flag_ck(0x58) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the 0xB8 event flag is up. */
u8 lb_progress_flag_b8_ck(void) {
    return userdata_progress_flag_ck(0xB8) == 1;
}

/* The lobby's "new area reached" gate: id 0's boundary, or id 20's boundary with mode 4 selected. */
u32 lb_event_state_ck(void) {
    if (lb_unlock_cond_last_ck(0) != 0) {
        return 1;
    }
    if (lb_unlock_cond_last_ck(20) != 0) {
        if ((u8)lb_world_mode_get() == 4) {
            return 1;
        }
    }
    return 0;
}

} /* extern "C" */

/* Whether the whale event is up; outside the block above because the map spells it mangled (C++ linkage, rule 9). */
u32 LbCheckKujiraEvent(void) {
    return lobby_w.kujira_0x15F == 1;
}

extern "C" {

/* The second event gate byte of the lobby work block, +0x161. */
u32 lb_event_gate2_ck(void) {
    return lobby_w.field_0x161 == 1;
}

/* Re-dresses the lobby NPCs with the village scene mode forced to 8 and the lobby's dress byte at 2, then restores
 * the mode to 0xFF. */
void lb_pig_dress_refresh(void) {
    lobby_world_block->mode_0x3E01 = 8;
    lobby_w.field_0x079 = 2;
    lb_npc_dress_refresh_all();
    stage_area_objects_refresh();
    lobby_world_block->mode_0x3E01 = 0xFF;
}

/* Whether id 5 is accepted. */
u8 lb_unlock_cond5_on_ck(void) {
    return lb_unlock_cond_ck(5) == 1;
}

/* The work block of the lobby torch effect (type 0x26): one pooled effect, the point light it lights and the torch
 * index.  size: 0x0C (the size `eft038_set` pools) */
struct Eft038Work {
    /* +0x00 */ s32 count_0x00;               /* the pooled effects in `effect_0x04` (1) */
    /* +0x04 */ nw4r::ef::Effect* effect_0x04;
    /* +0x08 */ u16 light_0x08;               /* the point light id `light_id_next` handed out */
    /* +0x0A */ u8 index_0x0A;                /* the torch index (0..5) */
    /* +0x0B */ u8 pad_0x0B;
};

/* The six lobby torch positions, by torch index. */
extern VEC3 eft038_torch_pos[6];

void eft038_effect_release(_EFT* self);
void eft038_dispatch(_EFT* self);
void eft038_create(_EFT* self);
void eft038_move(_EFT* self);
void eft038_fade(_EFT* self);
void eft038_release(_EFT* self);

/* Spawns the torch effect for torch `index` at its table position: torches 1..4 stand in area 4, torches 0 and 5 in
 * area 6. */
void eft038_set(u8 index) {
    VEC3 unused;
    _EFT* self;
    VEC3* pos;
    Eft038Work* work;

    VEC3_ctor(&unused);
    self = (_EFT*)eft_res_slot_get(12);
    if (self != NULL) {
        pos = &eft038_torch_pos[index];
        self->source_0x30 = NULL;
        work = (Eft038Work*)self->work_0x38;
        work->count_0x00 = 1;
        work->index_0x0A = index;
        self->field_0x03 = 38;
        self->type_0x02 = 0;
        switch (index) {
        case 0:
        case 5:
            self->area_0x44 = 6;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            self->area_0x44 = 4;
            break;
        default:
            eft038_release(self);
            return;
        }
        eft_state_flags_set(self, 1, 0);
        self->pos_0x18.x = pos->x;
        self->pos_0x18.y = pos->y;
        self->pos_0x18.z = pos->z;
        self->release_0x40 = eft038_effect_release;
        self->dispatch_0x34 = eft038_dispatch;
    }
}

/* Returns the torch's pooled effects to the effect heap. */
void eft038_effect_release(_EFT* self) {
    Eft038Work* work = (Eft038Work*)self->work_0x38;

    push_eft_effect_heap_num(&work->effect_0x04, work->count_0x00);
    work->count_0x00 = 0;
}

/* Steps the torch effect's state: create, burn, fade, release. */
void eft038_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft038_create(self);
        break;
    case 1:
        eft038_move(self);
        break;
    case 2:
        eft038_fade(self);
        break;
    case 3:
        eft038_release(self);
        break;
    }
}

/* Creates the flame effect and takes a point light for it, then runs the first burn step. */
void eft038_create(_EFT* self) {
    VEC3 unused;
    Eft038Work* work;

    VEC3_ctor(&unused);
    work = (Eft038Work*)self->work_0x38;
    self->state_0x05++;
    work->effect_0x04 = res_eft_create_ex(0x732, 60, 0, 200);
    if (work->effect_0x04 == NULL) {
        eft038_release(self);
        return;
    }
    self->flag_0x01 = 1;
    work->light_0x08 = light_id_next();
    eft038_move(self);
}

/* Burns while the torch is lit: in the torch's own area plays its sound, moves the flame and sets its light; once the
 * stage puts the torch out, starts the 30-frame fade. */
void eft038_move(_EFT* self) {
    VEC3 unused;
    Eft038Work* work;

    VEC3_ctor(&unused);
    work = (Eft038Work*)self->work_0x38;
    if (stage_torch_unlit_ck(work->index_0x0A) == 1) {
        self->state_0x05++;
        self->field_0x06 = 0;
        self->field_0x10 = 30;
        return;
    }
    if (self->area_0x44 == get_now_areano()) {
        shell_se_req(NULL, &self->pos_0x18, 15, work->index_0x0A);
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        effect_move(work->effect_0x04);
        if (get_demo_no() != 33) {
            light_point_scale_set(work->light_0x08, &self->pos_0x18, 1.0f, 1);
        }
        eft_res_models_spawn(self, (void**)&work->effect_0x04, 1, work->count_0x00, 0);
    }
}

/* Puts the flame out: stops its sound and retires its emitters, then lets the effect run down for `field_0x10`
 * frames before advancing to the release. */
void eft038_fade(_EFT* self) {
    Eft038Work* work = (Eft038Work*)self->work_0x38;

    switch (self->field_0x06) {
    case 0:
        se_torch_slot_req(work->index_0x0A);
        self->flag_0x01 = 1;
        self->field_0x06++;
        work->effect_0x04->RetireEmitterAll();
    case 1:
        effect_retire(work->effect_0x04, 0);
        if (--self->field_0x10 < 0) {
            self->state_0x05++;
            break;
        }
        eft_res_models_spawn(self, (void**)&work->effect_0x04, 1, work->count_0x00, 0);
        break;
    }
}

/* Releases the torch effect's pooled slot. */
void eft038_release(_EFT* self) {
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------------------------
 * em023: the program `em023_prog_tbl` runs.
 * ------------------------------------------------------------------------------------------------------------- */

/* The two turn sequences the em023 move actions run (`em_turn_seq_start`/`em_turn_seq_step`): four 16-byte steps. */
extern const u32 em023_turn_seq_a[16];
extern const u32 em023_turn_seq_b[16];

/* The per-sub-state sound programs of actions 10 and 11 (`em_se_tbl_play` / `em_se_tbl_play_alt`). */
extern EmSeEntry em023_se_seq_0a[];
extern EmSeEntry em023_se_seq_30[];
extern EmSeEntry em023_se_seq_31[];
extern EmSeEntry em023_se_seq_89[];
extern EmSeEntry em023_se_seq_8a[];
extern EmSeEntry em023_se_seq_8b[];
extern EmSeEntry em023_se_seq_8c[];
extern EmSeEntry em023_se_seq_8f[];
extern EmSeEntry em023_se_seq_a1[];
extern EmSeEntry em023_se_seq_b1[];
extern EmSeEntry em023_se_alt_seq_00[];
extern EmSeEntry em023_se_alt_seq_3b[];

/* The route the action-12 walk follows per map kind and area: the move-target ids (0xFF ends a route) and the heading
 * each leg starts with. */
extern u8 em023_route_m1_a07[4];
extern u8 em023_route_m1_a12[4];
extern u8 em023_route_m3_a02[4];
extern u8 em023_route_m3_a08[4];
extern f32 em023_route_ang_m1_a07[4];
extern f32 em023_route_ang_m1_a12[4];
extern f32 em023_route_ang_m3_a02[1];
extern f32 em023_route_ang_m3_a08[1];

} /* extern "C" */

/* A vector copied in from the one `setVec3` builds, so a file-scope one is constructed by MWCC's `__sinit`.
 * size: 0xC */
struct Em023SeVec : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    Em023SeVec(f32 x, f32 y, f32 z)
    {
        nw4r::math::VEC3 v;

        assignVec3((Vec*)this, (Vec*)setVec3(&v, x, y, z));
    }
};

/* The two sound-position offsets two of the action-10 sound records point at (an offset and a direction each). */
extern "C" Em023SeVec em023_se_ofs_a[2] = {
    Em023SeVec(0.0f, 6.0f, 0.0f),
    Em023SeVec(0.0f, 0.0f, 0.0f),
};
extern "C" Em023SeVec em023_se_ofs_b[2] = {
    Em023SeVec(0.0f, -3.0f, 0.0f),
    Em023SeVec(0.0f, 0.0f, 0.0f),
};

extern "C" {

/* The demo-5 key tables `em_demo_key3_apply`/`em_demo_key_apply` interpolate (position keys, rotation keys). */
extern u8 em023_demo5_keys_a[];
extern u8 em023_demo5_keys_b[];
extern u8 em023_demo5_keys_c[];
extern u8 em023_demo5_keys_d[];
extern u8 em023_demo5_keys_e[];
extern u8 em023_demo5_keys_f[];

u32 em023_stage_actor_busy_ck(u32 area);

/* Converts degrees to the 16-bit angle unit. */
#define EM023_DEG2ANG(deg) ((u16)(s32)(0.5f + (65536.0f * (deg)) / 360.0f))

/* Clears the program's five state bytes at +0x328. */
void em023_work_clear(_ENEMY_WORK* self) {
    self->em023_0x328.leg_0x329 = 0;
    self->em023_0x328.flag_0x328 = 0;
    self->em023_0x328.synced_0x32B = 0;
    self->em023_0x328.slot_state_0x32A = 0;
    self->em023_0x328.arrived_0x32C = 0;
}

/* Starts the program: raises status bit 0x80 when the work carries flag 8, and on a fresh start arms the idle
 * action in move mode 2. */
void em023_init(_ENEMY_WORK* self, u8 mode) {
    if (self->field_0x1C8 & 8) {
        em_status_bits_set(self, 0x80);
    }
    if (mode == 0) {
        em_move_mode_set(self, 2);
        em_act_arm_unless_down(self, 0, 4);
    }
}

/* Picks the follow-up of action 0 sub 4: action 5 sub 15 once the enemy has risen to its spawn height and its
 * +0x328 flag is set. */
void em023_motion_next(_ENEMY_WORK* self, u8* action, u8* sub) {
    if (*action == 0) {
        switch (*sub) {
        case 4:
            if (self->pos.y >= -100.0f + self->field_0x210 && self->em023_0x328.flag_0x328 != 0) {
                *action = 5;
                *sub = 15;
            }
            break;
        }
    }
}

/* Reacts to the program events: clears the sync flag, latches the +0x328 flag, and on event 11 sub 59 places the demo
 * camera for the two stage maps the enemy appears in. */
void em023_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub) {
    switch (kind) {
    case 7:
        if (sub == 0 && self->em023_0x328.synced_0x32B == 1) {
            self->em023_0x328.synced_0x32B = 0;
        }
        break;
    case 5:
        switch (sub) {
        case 14:
            self->em023_0x328.flag_0x328 = 0;
            break;
        case 19:
            self->em023_0x328.flag_0x328 = 1;
            break;
        }
        break;
    case 13:
        if (sub == 0) {
            self->em023_0x328.synced_0x32B = 0;
            self->em023_0x328.slot_state_0x32A = 0;
        }
        break;
    case 11:
        self->em023_0x328.timer_0x332 = 0;
        switch (sub) {
        case 59:
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 7:
                    em_demo_pos_set(self, 512.0f, -2545.0f, 1284.0f);
                    em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 8:
                    em_demo_pos_set(self, -368.0f, -193.0f, -1372.0f);
                    em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
                    break;
                }
                break;
            }
            break;
        }
        break;
    }
}

/* Steps the enemy-data slot state the enemy is bound to: 0 waits for the slot to latch (or syncs the marks and the hit
 * reaction), 1 waits for the enemy to stand in the slot's shape, 2 waits for the slot to clear. */
void em023_slot_state_step(_ENEMY_DATA* data, _ENEMY_WORK* self) {
    switch (self->em023_0x328.slot_state_0x32A) {
    case 0:
        if (self->field_0x833 == 1) {
            ((void (*)(_ENEMY_DATA*, u32, u32))eft_slot_marks_set_sync)(data, 1, self->area_no);
            self->em023_0x328.synced_0x32B = 1;
            break;
        }
        if (data->field_0x0B == 1 && em_die_ck(self) == 0) {
            self->em023_0x328.slot_state_0x32A = 1;
            self->em023_0x328.arrived_0x32C = 0;
            if (data->work_0x18 == self) {
                self->em023_0x328.synced_0x32B = 1;
            }
            if (self->field_0x43B == 0) {
                switch (self->action) {
                case 5:
                    switch (self->state_sub) {
                    case 1:
                    case 3:
                    case 14:
                    case 22:
                        em_hit_by_set(self, ran_suu(0) & 0x1F, 0);
                        break;
                    }
                    break;
                }
            }
        }
        break;
    case 1:
        if (em_die_ck(self) == 0 &&
            eft_slot_shape_hit_ck((EftSlot*)data, data->field_0x36, &self->pos, 0.0f) != 0 && data->field_0x0A != 2) {
            self->em023_0x328.slot_state_0x32A = 2;
        }
        break;
    default:
        if (em_die_ck(self) == 0 && data->field_0x0B == 0) {
            self->em023_0x328.slot_state_0x32A = 0;
            self->em023_0x328.synced_0x32B = 0;
        }
        break;
    }
}

/* Follows the bound enemy-data slot: copies its aim vector and switches the motion kind as the slot's mode changes. */
void em023_slot_update(_ENEMY_DATA* data, _ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        copyVec3(&self->aim, &data->vec_0x24);
    }
    switch (data->field_0x08) {
    case 2:
        if (em_die_ck(self) == 0 && self->field_0x43B != 3) {
            self->em023_0x328.flag_0x328 = 0;
            self->em023_0x328.arrived_0x32C = 0;
            if (data->work_0x18 == self) {
                em_motion_kind_set(self, 3, 1);
            } else {
                em_motion_kind_set(self, 3, 0);
            }
        }
        em023_slot_state_step(data, self);
        break;
    case 3:
        if (em_die_ck(self) == 0 && self->field_0x43B != 2) {
            self->em023_0x328.slot_state_0x32A = 0;
            self->em023_0x328.synced_0x32B = 0;
            self->em023_0x328.arrived_0x32C = 0;
            em_motion_kind_set(self, 2, 0);
            em_hit_by_set(self, ran_suu(0) & 0x1F, 0);
        }
        break;
    case 0:
        if (em_die_ck(self) == 0 && (self->field_0x43B != 0 || self->field_0x43C != data->field_0x36)) {
            self->em023_0x328.flag_0x328 = 0;
            self->em023_0x328.arrived_0x32C = 0;
            em_motion_kind_set(self, 0, data->field_0x36);
        }
        em023_slot_state_step(data, self);
        break;
    }
}

/* The per-frame update: eases the surface clamp in over 60 frames of action 11, stops a busy enemy when a stage actor
 * of its area is in its action-13 opening, and follows the bound enemy-data slot. */
void em023_frame_update(_ENEMY_WORK* self) {
    _ENEMY_DATA* data;
    f32 clamp;

    if (self->action == 11) {
        if (++self->em023_0x328.timer_0x332 <= 60) {
            clamp = 1.3333334f * self->em023_0x328.timer_0x332;
        } else {
            clamp = 80.0f;
        }
        em_surface_clamp(self, clamp);
    }
    if (em_die_ck(self) == 0 && (self->field_0x1C8 & 8) && em_busy_ck(self) == 1 && em_act_ck(self, 13, 0) == 0 &&
        em023_stage_actor_busy_ck(self->area_no) == 1) {
        em_hit_by_set(self, 0, 1);
        self->em023_0x328.slot_state_0x32A = 0;
        self->em023_0x328.synced_0x32B = 0;
        return;
    }
    data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
    if (data != NULL) {
        em023_slot_update(data, self);
    }
}

/* Plays motion 1 once. */
void em023_idle_mot1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 1 once, blended in. */
void em023_idle_mot1_blend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 1, 10, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Action 0's sub-state dispatcher. */
void em023_idle_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em023_idle_mot1(self);
        break;
    case 1:
        em023_idle_mot1(self);
        break;
    case 2:
        em023_idle_mot1(self);
        break;
    case 4:
        em023_idle_mot1_blend(self);
        break;
    }
}

/* Plays motion 1 blended in (move action). */
void em023_move_mot1_blend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 1, 10, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Runs the first turn sequence; with `follow` set the enemy keeps to its follow height meanwhile. */
void em023_move_turn_seq(_ENEMY_WORK* self, u8 follow) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em023_turn_seq_a, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em023_turn_seq_a) == 1) {
            em_action_finish_swim(self);
            break;
        }
        if (follow == 1) {
            if (self->field_0x482 != 0) {
                em_height_follow(self, 5.0f);
            } else {
                em_height_follow(self, 10.0f);
            }
        }
        break;
    }
}

/* Plays motion 4 once. */
void em023_move_mot4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 4, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Walks to the move target on motion 9 (`mode` 1/3 start it at frame 62, 2/3 speed it up by 1.5); in mode 2 it
 * marks arrival once it stands in its slot's shape, or within 400 units of its aim point without a slot. */
void em023_move_approach(_ENEMY_WORK* self, u8 mode) {
    _ENEMY_DATA* data;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        if (mode == 0) {
            em_mot_set(self, 9, 10, 0);
        } else if (mode == 1) {
            em_mot_set(self, 9, 0, 62);
        } else if (mode == 2) {
            em_mot_set(self, 9, 10, 0);
            em_mot_speed_set(self, 1.5f);
        } else {
            em_mot_set(self, 9, 0, 62);
            em_mot_speed_set(self, 1.5f);
        }
        em_approach_start(self, 0.0f, 2);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            if (mode == 2 && self->em023_0x328.arrived_0x32C == 0) {
                data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
                if (data != NULL) {
                    if (eft_slot_shape_hit_ck((EftSlot*)data, data->field_0x36, &self->pos, 0.0f) != 0) {
                        self->em023_0x328.arrived_0x32C = 1;
                    }
                } else if (vec3_dist_sq(&self->pos, &self->aim) <= 160000.0f) {
                    self->em023_0x328.arrived_0x32C = 1;
                }
            }
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Descends on motion 9 while turning to its heading until it touches the ground (then lands on motions 5, 6, 7), or
 * gives up after 600 frames. */
void em023_move_descend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 10, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        em_hit_arm_set(self);
        self->field_0x1BC = em_angle_approach(0x2AAB, self->field_0x1BC, 182);
        self->timer_0x020++;
        if (((u32 (*)(_ENEMY_WORK*, f32))em_ground_ck)(self, 560.0f) == 1) {
            em_mot_set(self, 5, 10, 0);
            self->state++;
            break;
        }
        if (self->timer_0x020 >= 600) {
            em_action_finish_swim(self);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 6, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0);
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 8 once. */
void em023_move_mot8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 8, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x68 once. */
void em023_move_mot68(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Walks to the move target on motion 9 at speed 1.5. */
void em023_move_approach_fast(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 10, 0);
        em_mot_speed_set(self, 1.5f);
        em_approach_start(self, 0.0f, 2);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x0E once. */
void em023_move_mot0e(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xE, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Walks to the move target on motion 12 (`mode` 1 starts it at frame 50) and marks arrival like
 * `em023_move_approach`'s mode 2. */
void em023_move_approach_c(_ENEMY_WORK* self, u8 mode) {
    _ENEMY_DATA* data;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        if (mode == 0) {
            em_mot_set(self, 0xC, 0, 0);
        } else {
            em_mot_set(self, 0xC, 0, 50);
        }
        em_approach_start(self, 0.0f, 2);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            if (self->em023_0x328.arrived_0x32C == 0) {
                data = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
                if (data != NULL) {
                    if (eft_slot_shape_hit_ck((EftSlot*)data, data->field_0x36, &self->pos, 0.0f) != 0) {
                        self->em023_0x328.arrived_0x32C = 1;
                    }
                } else if (vec3_dist_sq(&self->pos, &self->aim) <= 160000.0f) {
                    self->em023_0x328.arrived_0x32C = 1;
                }
            }
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x0F while turning to the target. */
void em023_move_turn_mot0f(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        em_turn_to_target(self, 0x16C);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x10 while walking to the move target; the end of the motion switches to action 5 sub 24. */
void em023_move_mot10_approach(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x10, 6, 0);
        em_approach_start(self, 0.0f, 2);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x18);
            break;
        }
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Walks to the move target on motion 9 at speed 1.5 with the far approach reach. */
void em023_move_approach_far(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 10, 0);
        em_mot_speed_set(self, 1.5f);
        em_approach_start(self, -400.0f, 2);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Climbs on motion 9 while turning to its heading until it reaches its spawn height (then action 5 sub 19), or gives
 * up after 600 frames. */
void em023_move_climb(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 10, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        self->field_0x1BC = em_angle_approach(0xD555, self->field_0x1BC, 182);
        self->timer_0x020++;
        if (self->pos.y >= -100.0f + self->field_0x210) {
            em_state_set(self, 5, 0x13);
            break;
        }
        if (self->timer_0x020 >= 600) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x21 (`mode` 1 at speed 1.5), turning to heading 0x2000 inside its frame window. */
void em023_move_mot21(_ENEMY_WORK* self, u8 mode) {
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x21, 10, 0);
        if (mode == 1) {
            em_mot_speed_set(self, 1.5f);
        }
        break;
    case 1:
        if (em_frame_check(self, 3, 10.0f, 50.0f) == 1) {
            self->field_0x1BC = em_angle_approach(0x2000, self->field_0x1BC, 0x222);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x1E blended in; while the +0x328 flag is set the enemy rises 5 units a frame to its spawn height. */
void em023_move_rise_blend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 0x1E, 10, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
    if (self->em023_0x328.flag_0x328 != 0) {
        if (self->pos.y < self->field_0x210) {
            self->pos.y += 5.0f;
            if (self->pos.y > self->field_0x210) {
                self->pos.y = self->field_0x210;
            }
        }
    }
}

/* Walks to the move target on motion 0x1F; while the +0x328 flag is set the enemy rises to its spawn height. */
void em023_move_rise_approach(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x1F, 10, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
    if (self->em023_0x328.flag_0x328 != 0) {
        if (self->pos.y < self->field_0x210) {
            self->pos.y += 5.0f;
            if (self->pos.y > self->field_0x210) {
                self->pos.y = self->field_0x210;
            }
        }
    }
}

/* Plays motion 5 blended in. */
void em023_move_mot5_blend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 5, 10, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x20 blended in: turns to heading 0 four degrees a frame inside its frame window and rises 2 units a
 * frame to its spawn height, landing on it when the motion ends. */
void em023_move_rise_turn(_ENEMY_WORK* self) {
    f32 step = 4.0f;

    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set_blend(self, 0x20, 40, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
            self->field_0x1BC = em_angle_approach(0, self->field_0x1BC, EM023_DEG2ANG(step));
        }
        if (self->pos.y < self->field_0x210) {
            self->pos.y += 2.0f;
            if (self->pos.y > self->field_0x210) {
                self->pos.y = self->field_0x210;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->pos.y = self->field_0x210;
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Plays motion 0x0F; `mode` 1 looks around: picks a random heading within 45 degrees and a pitch within 90 and turns
 * to them inside the frame window.  The end of the motion switches to action 5 sub 21 (mode 0) or 28. */
void em023_move_look(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xF, 6, 0);
        if (mode == 1) {
            self->em023_0x328.aim_x_0x32E = self->field_0x1BC + EM023_DEG2ANG((f32)(self->bits_0x1EC % 90 - 45));
            self->em023_0x328.aim_y_0x330 = self->field_0x1C0 + EM023_DEG2ANG((f32)(self->bits_0x1EC % 180 - 90));
        }
        break;
    case 1:
        if (mode == 1 && em_frame_check(self, 3, 50.0f, 100.0f) == 1) {
            self->field_0x1BC = em_angle_approach(self->em023_0x328.aim_x_0x32E, self->field_0x1BC, 0x28F);
            self->field_0x1C0 = em_angle_approach(self->em023_0x328.aim_y_0x330, self->field_0x1C0, 0x28F);
        }
        if (em_mot_end_ck(self) == 1) {
            if (mode == 0) {
                em_state_set(self, 5, 0x15);
            } else {
                em_state_set(self, 5, 0x1C);
            }
        }
        break;
    }
}

/* Plays motion 0x10; `mode` 0 waits for its end, `mode` 1 waits 60..75 frames; then action 5 sub 18. */
void em023_move_wait(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x10, 6, 0);
        if (mode == 1) {
            self->timer_0x020 = (self->bits_0x1EC & 0xF) + 60;
        }
        break;
    case 1:
        if (mode == 0) {
            if (em_mot_end_ck(self) == 1) {
                em_state_set(self, 5, 0x12);
            }
        } else if (--self->timer_0x020 <= 0) {
            em_state_set(self, 5, 0x12);
        }
        break;
    }
}

/* Plays motion 0x0A blended in. */
void em023_move_mot0a_blend(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 0xA, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Runs the second turn sequence. */
void em023_move_turn_seq_b(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em023_turn_seq_b, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em023_turn_seq_b) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Idles on motion 1 for up to 14 frames, then action 5 sub 27. */
void em023_move_pause(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 20, 0);
        self->timer_0x020 = self->bits_0x1EC % 15;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set(self, 5, 0x1B);
        }
        break;
    }
}

/* Turns away from its target by an eighth of a turn to a random side through the first turn sequence. */
void em023_move_turn_away(_ENEMY_WORK* self) {
    s32 angle;
    s32 heading;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        angle = calcVecAng2(&self->pos, &self->vec_0x36C);
        heading = angle;
        if (self->bits_0x1EC & 1) {
            heading += 0x2000;
        } else {
            heading -= 0x2000;
        }
        em_turn_seq_start(self, (void*)em023_turn_seq_a, 2, 0, (u16)heading);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em023_turn_seq_a) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Action 5's sub-state dispatcher (the move actions). */
void em023_move_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em023_move_mot1_blend(self);
        break;
    case 1:
        em023_move_turn_seq(self, 0);
        break;
    case 2:
        em023_move_mot4(self);
        break;
    case 3:
        em023_move_approach(self, 0);
        break;
    case 4:
        em023_move_descend(self);
        break;
    case 5:
        em023_move_mot8(self);
        break;
    case 6:
        em023_move_mot68(self);
        break;
    case 7:
        em023_move_approach_fast(self);
        break;
    case 8:
        em023_move_mot0e(self);
        break;
    case 9:
        em023_move_approach_c(self, 0);
        break;
    case 10:
        em023_move_turn_mot0f(self);
        break;
    case 11:
        em023_move_mot10_approach(self);
        break;
    case 12:
        em023_move_approach_far(self);
        break;
    case 13:
        em023_move_climb(self);
        break;
    case 14:
        em023_move_mot21(self, 0);
        break;
    case 15:
        em023_move_rise_blend(self);
        break;
    case 16:
        em023_move_rise_approach(self);
        break;
    case 17:
        em023_move_turn_seq(self, 1);
        break;
    case 18:
        em023_move_mot5_blend(self);
        break;
    case 19:
        em023_move_rise_turn(self);
        break;
    case 20:
        em023_move_look(self, 0);
        break;
    case 21:
        em023_move_wait(self, 0);
        break;
    case 22:
        em023_move_approach(self, 1);
        break;
    case 23:
        em023_move_mot0a_blend(self);
        break;
    case 24:
        em023_move_approach_c(self, 1);
        break;
    case 25:
        em023_move_approach(self, 2);
        break;
    case 26:
        em023_move_turn_seq_b(self);
        break;
    case 27:
        em023_move_look(self, 1);
        break;
    case 28:
        em023_move_wait(self, 1);
        break;
    case 29:
        em023_move_pause(self);
        break;
    case 30:
        em023_move_turn_away(self);
        break;
    case 31:
        em023_move_mot21(self, 1);
        break;
    case 32:
        em023_move_approach(self, 3);
        break;
    }
}

/* Plays motion 0x28 with the default hit window. */
void em023_act7_mot28(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 10, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Action 7's sub-state dispatcher. */
void em023_act7_dispatch(_ENEMY_WORK* self) {
    if (self->state_sub != 0) {
        return;
    }
    em023_act7_mot28(self);
}

/* Action 10: plays the sound program of the current sub-state, or ends the action. */
void em023_sound_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x0A:
        em_se_tbl_play(self, em023_se_seq_0a, 2, 0x0A);
        break;
    case 0x30:
        em_se_tbl_play(self, em023_se_seq_30, 2, 0x30);
        break;
    case 0x31:
        em_se_tbl_play(self, em023_se_seq_31, 2, 0x31);
        break;
    case 0x89:
        em_se_tbl_play(self, em023_se_seq_89, 2, 0x89);
        break;
    case 0x8A:
        em_se_tbl_play(self, em023_se_seq_8a, 2, 0x8A);
        break;
    case 0x8B:
        em_se_tbl_play(self, em023_se_seq_8b, 2, 0x8B);
        break;
    case 0x8C:
        em_se_tbl_play(self, em023_se_seq_8c, 2, 0x8C);
        break;
    case 0x8F:
        em_se_tbl_play(self, em023_se_seq_8f, 2, 0x8F);
        break;
    case 0x90:
        em_se_tbl_play(self, em023_se_seq_8f, 2, 0x90);
        break;
    case 0xA1:
        em_se_tbl_play(self, em023_se_seq_a1, 2, 0xA1);
        break;
    case 0xA2:
        em_se_tbl_play(self, em023_se_seq_8f, 2, 0xA2);
        break;
    case 0xAA:
        em_se_tbl_play(self, em023_se_seq_0a, 2, 0xAA);
        break;
    case 0xB1:
        em_se_tbl_play(self, em023_se_seq_b1, 2, 0xB1);
        break;
    default:
        em_action_finish_swim(self);
        break;
    }
}

/* Action 11: plays the alternate sound program of the current sub-state. */
void em023_sound_alt_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x00:
        em_se_tbl_play_alt(self, em023_se_alt_seq_00, 2, 0x00);
        break;
    case 0x0A:
        em_se_tbl_play_alt(self, em023_se_alt_seq_00, 2, 0x0A);
        break;
    case 0x13:
        em_se_tbl_play_alt(self, em023_se_alt_seq_00, 2, 0x13);
        break;
    case 0x3B:
        em_se_tbl_play_alt(self, em023_se_alt_seq_3b, 2, 0x3B);
        break;
    default:
        em_se_tbl_play_alt(self, em023_se_alt_seq_00, 2, 0x0A);
        break;
    }
}

/* Action 12: walks the area's route leg by leg - each arrival sets the next move target and heading and restarts the
 * action, the end of the route (or an area without one) ends it. */
void em023_route_step(_ENEMY_WORK* self) {
    u8* route;
    f32* angles;

    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 10, 0);
        em_approach_start(self, 0.0f, 2);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            switch (stage_map_kind_get(self->field_0x1E0)) {
            case 1:
                switch (self->area_no) {
                case 7:
                    route = em023_route_m1_a07;
                    angles = em023_route_ang_m1_a07;
                    break;
                case 12:
                    route = em023_route_m1_a12;
                    angles = em023_route_ang_m1_a12;
                    break;
                default:
                    route = NULL;
                    angles = NULL;
                    break;
                }
                break;
            case 3:
                switch (self->area_no) {
                case 2:
                    route = em023_route_m3_a02;
                    angles = em023_route_ang_m3_a02;
                    break;
                case 8:
                    route = em023_route_m3_a08;
                    angles = em023_route_ang_m3_a08;
                    break;
                default:
                    route = NULL;
                    angles = NULL;
                    break;
                }
                break;
            default:
                route = NULL;
                angles = NULL;
                break;
            }
            if (route != NULL && angles != NULL) {
                if (route[self->em023_0x328.leg_0x329] != 0xFF) {
                    em_move_target_set(self, 0xFF, route[self->em023_0x328.leg_0x329],
                                       angles[self->em023_0x328.leg_0x329]);
                    self->em023_0x328.leg_0x329++;
                    em_state_set(self, 12, 0);
                } else {
                    self->em023_0x328.leg_0x329 = 0;
                    em_action_finish_swim(self);
                }
            } else {
                self->em023_0x328.leg_0x329 = 0;
                em_action_finish_swim(self);
            }
        }
        em_frame_flag_set(self);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    }
}

/* Action 12's sub-state dispatcher. */
void em023_route_dispatch(_ENEMY_WORK* self) {
    if (self->state_sub != 0) {
        return;
    }
    em023_route_step(self);
}

/* Demo sub 0: plays motion 4, then holds for action 13 while a stage actor of the area is in its opening. */
void em023_demo_wait(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 4, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (em023_stage_actor_busy_ck(self->area_no) == 1) {
                em_state_set(self, 13, 0);
            } else {
                em_action_finish_swim(self);
            }
        }
        break;
    }
}

/* Demo sub 1: the first camera demo's key frames (frames 500, 540, 636, 768). */
void em023_demo1(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 0, 90);
        em_demo_pos_set(self, -4905.7f, -2343.0f, -902.0f);
        em_demo_rot_set(self, -7.0f, 100.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(500) != 0) {
            self->state++;
            em_mot_set(self, 9, 0, 150);
            em_demo_pos_set(self, 404.5f, -2077.8f, 12.7f);
            em_demo_rot_set(self, -7.0f, 23.5f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(540) != 0) {
            self->state++;
            em_demo_reset(self, NULL);
        }
        break;
    case 3:
        if (em_demo_time_ck(636) != 0) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0x10, 0, 20);
            em_demo_pos_set(self, -1426.0f, -2464.0f, -805.0f);
            em_demo_rot_set(self, 0.0f, -160.0f, 0.0f);
        }
        break;
    case 4:
        if (em_demo_time_ck(768) != 0) {
            self->state++;
            em_demo_reset(self, NULL);
        }
        break;
    }
}

/* Demo sub 2: plays motion 1 at the second demo position. */
void em023_demo2(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, 3000.0f, -2400.0f, -3900.0f);
        em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* Demo sub 3: the third camera demo's key frames (frames 500, 540, 636, 768). */
void em023_demo3(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 0, 150);
        em_demo_pos_set(self, -5043.0f, -2670.0f, -577.0f);
        em_demo_rot_set(self, 0.0f, 100.0f, 0.0f);
        break;
    case 1:
        if (em_demo_time_ck(500) != 0) {
            self->state++;
            em_mot_set(self, 9, 0, 140);
            em_demo_pos_set(self, -710.8f, -2002.0f, -1089.0f);
            em_demo_rot_set(self, -10.0f, 32.0f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(540) != 0) {
            self->state++;
            em_mot_set(self, 0x6C, 0, 16);
            em_demo_pos_set(self, 489.0f, -2085.0f, -517.0f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
        }
        break;
    case 3:
        if (em_demo_time_ck(636) != 0) {
            self->state++;
            em_mot_set(self, 0x70, 0, 304);
            em_demo_pos_set(self, -410.0f, -2036.0f, -1027.0f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
        }
        break;
    case 4:
        if (em_demo_time_ck(768) != 0) {
            self->state++;
            em_mot_set(self, 0x71, 0, 0);
            em_demo_pos_set(self, -410.0f, -2036.0f, -1027.0f);
            em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
        }
        break;
    }
}

/* Demo sub 4: places the enemy at the map-1 demo position and starts action 11 sub 56. */
void em023_demo4_start(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    em_move_mode_set(self, 2);
    em_demo_pos_set(self, 512.0f, -2545.0f, 1284.0f);
    em_demo_rot_set(self, 0.0f, -12.0f, 0.0f);
    em_state_set(self, 11, 0x38);
}

/* Demo sub 5: the long camera demo - key frames from 382 to 1418, the splash effects on the motion frames and the
 * keyed position/rotation tracks. */
void em023_demo5(_ENEMY_WORK* self) {
    VEC3 ofs;

    VEC3_ctor(&ofs);
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 9, 0, 60);
        em_demo_pos_set(self, 1321.0f, -556.0f, -3320.3f);
        em_demo_rot_set(self, 4.0f, -90.0f, 0.0f);
        break;
    case 1:
        if (em_frame_check(self, 0, 152.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 20.0f);
            eft_em_spawn(self, 0xCC, 0x13, &ofs, 0.7f);
        }
        if (em_demo_time_ck(382) != 0) {
            self->state++;
            em_mot_set(self, 9, 0, 0);
            em_demo_pos_set(self, 835.7f, -1060.0f, -2950.0f);
            em_demo_rot_set(self, 6.0f, -90.0f, 0.0f);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 20.0f);
            eft_em_spawn(self, 0xCC, 0x13, &ofs, 0.7f);
        }
        if (em_demo_time_ck(778) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0, 0);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(874) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(910) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 5:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(946) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 6:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(982) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 7:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(1018) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 8:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(1054) != 0) {
            self->state++;
            em_mot_set(self, 0xF, 0x10, 0x4C);
        }
        break;
    case 9:
        if (em_frame_check(self, 3, 76.0f, 94.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&ofs, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 3, &ofs, 1.6f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_a, em023_demo5_keys_b);
        if (em_demo_time_ck(1096) != 0) {
            self->state++;
            em_mot_set(self, 0x70, 0, 0x16C);
        }
        break;
    case 10:
        if (em_frame_check(self, 0, 404.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, -30.0f, 60.0f);
            eft_em_spawn(self, 3, 0x14, &ofs, 0.7f);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em023_demo5_keys_c, em023_demo5_keys_d);
        if (em_demo_time_ck(1206) != 0) {
            self->state++;
            em_mot_set(self, 0x70, 0, 0);
            em_demo_rot_set(self, 169.4f, 251.15f, 190.8f);
        }
        break;
    case 11:
        em_demo_key_apply(self, em_demo_frame_get(), em023_demo5_keys_e, em023_demo5_keys_f, 7, 2);
        if (em_demo_time_ck(1418) != 0) {
            self->state++;
            em_demo_reset(self, NULL);
        }
        break;
    }
}

/* Demo sub 6: places the enemy at the map-3 demo position and starts action 11 sub 56. */
void em023_demo6_start(_ENEMY_WORK* self) {
    em_hit_arm_set(self);
    em_demo_enable(self);
    em_move_mode_set(self, 2);
    em_demo_pos_set(self, -368.0f, -193.0f, -1372.0f);
    em_demo_rot_set(self, 0.0f, 0.0f, 0.0f);
    em_state_set(self, 11, 0x38);
}

/* Action 13's sub-state dispatcher (the demos). */
void em023_demo_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em023_demo_wait(self);
        break;
    case 1:
        em023_demo1(self);
        break;
    case 2:
        em023_demo2(self);
        break;
    case 3:
        em023_demo3(self);
        break;
    case 4:
        em023_demo4_start(self);
        break;
    case 5:
        em023_demo5(self);
        break;
    case 6:
        em023_demo6_start(self);
        break;
    }
}

/* The program's action dispatcher. */
void em023_action_dispatch(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em023_idle_dispatch(self);
        break;
    case 5:
        em023_move_dispatch(self);
        break;
    case 7:
        em023_act7_dispatch(self);
        break;
    case 10:
        em023_sound_dispatch(self);
        break;
    case 11:
        em023_sound_alt_dispatch(self);
        break;
    case 12:
        em023_route_dispatch(self);
        break;
    case 13:
        em023_demo_dispatch(self);
        break;
    }
}

/* The motion-event hook: returns the current motion number. */
void em023_motion_events(_ENEMY_WORK* self) {
    em_get_mot_no(self);
}

/* The TEV colour hook (nothing to do). */
void em023_tev_color(_ENEMY_WORK* self) {
}

/* Answers the program's state queries: 0 risen to the spawn height, 1 slot latched, 2 synced, 3 arrived. */
u8 em023_state_query(_ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        return self->pos.y >= -100.0f + self->field_0x210;
    case 1:
        return self->em023_0x328.slot_state_0x32A == 1;
    case 2:
        return self->em023_0x328.synced_0x32B;
    case 3:
        return self->em023_0x328.arrived_0x32C;
    default:
        return 0;
    }
}

/* Picks the action the enemy starts in: the route walk (action 12) in the two map areas that have a route, facing
 * the area's start heading; otherwise action 0 sub 4 at the origin. */
void em023_action_select(_ENEMY_WORK* self, u8* action, u8* sub) {
    self->em023_0x328.leg_0x329 = 0;
    em_move_mode_set(self, 2);
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        *action = 12;
        *sub = 0;
        switch (self->area_no) {
        case 7:
            em_move_target_set(self, 0, 1, 0.0f);
            self->field_0x1C0 = 0x5800;
            break;
        case 12:
            em_move_target_set(self, 0, 1, 0.0f);
            self->field_0x1C0 = 0x3400;
            break;
        default:
            *action = 0;
            *sub = 4;
            setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
            self->field_0x1C4 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1BC = 0;
            break;
        }
        break;
    case 3:
        *action = 12;
        *sub = 0;
        switch (self->area_no) {
        case 2:
            em_move_target_set(self, 0, 1, 0.0f);
            self->field_0x1C0 = 0xF400;
            break;
        case 8:
            em_move_target_set(self, 0, 1, 0.0f);
            self->field_0x1C0 = 0xC00;
            break;
        default:
            *action = 0;
            *sub = 4;
            setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
            self->field_0x1C4 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1BC = 0;
            break;
        }
        break;
    default:
        *action = 0;
        *sub = 4;
        setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        break;
    }
}

/* The action re-arm hook (nothing to do). */
void em023_action_rearm(_ENEMY_WORK* self) {
}

/* Whether a live em015 or em018 in area `area` is in the first five sub-states of its action 13. */
u32 em023_stage_actor_busy_ck(u32 area) {
    s32 i;
    _ENEMY_WORK* work;
    u16 max;

    max = get_move_work_max(3);
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->area_no == (u8)area) {
            switch (work->team) {
            case 15:
                if (em015_act13_early_ck(work) == 1) {
                    return 1;
                }
                break;
            case 18:
                if (em018_act13_early_ck(work) == 1) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

/* Whether the whale-event work's countdown byte has gone negative. */
u32 kujira_event_over_ck(void) {
    return lobby_world_block->work_0x4832.count_0x02 < 0;
}

/* Re-opens the whale-event work: clears the bit field, mirrors it into the flag byte, draws a new
 * seed and seeds the countdown. */
void lb_kujira_work_reset(void) {
    LbKujiraWork* work = &lobby_world_block->work_0x4832;

    work->bits_0x01 = 0;
    lb_kujira_bit_set(0);
    work->flag_0x00 = work->bits_0x01;
    lb_kujira_seed_roll(work);
    work->count_0x02 = -4;
}

/* Sets the `kind`-th bit of the whale-event work's bit field. */
void lb_kujira_bit_set(u32 raw) {
    lobby_world_block->work_0x4832.bits_0x01 |= (u8)(1 << (u8)raw);
}

/* The whale event's daily step: counts the visit down towards 0; when a stay ends it draws the next seed and an
 * absence from the range the unlocked bits select (shortened by the item it was given), when an absence ends it
 * starts a four-day stay and unlocks the progress entries the village rank has reached. */
void lb_kujira_day_step(void) {
    LbKujiraWork* work = &lobby_world_block->work_0x4832;
    s32 step;
    u8 n;
    s32 i;
    u8 lo;
    u8 hi;

    if (work->count_0x02 != 0) {
        step = 1;
        if (work->count_0x02 > 0) {
            step = -1;
        }
        work->count_0x02 = work->count_0x02 + step;
        if (work->count_0x02 == 0) {
            if (step > 0) {
                lb_kujira_seed_roll(work);
                n = 0;
                work->flag_0x00 = work->bits_0x01;
                for (i = 0; i < 3; i++) {
                    if (work->flag_0x00 & (1 << i)) {
                        n++;
                    }
                }
                if (n != 0) {
                    n--;
                }
                lo = lb_kujira_stay_ranges[n].min_0x00;
                hi = lb_kujira_stay_ranges[n].max_0x01;
                work->count_0x02 = lo + ran_suu(1) % (hi - lo);
                switch (work->item_0x06) {
                case 0xFB:
                    work->count_0x02 -= 1;
                    break;
                case 0xED:
                    work->count_0x02 -= 2;
                    break;
                }
                work->item_0x06 = 0;
            } else {
                work->field_0x03 = 0;
                work->count_0x02 = -4;
                if (lb_unlock_cond_ck(12) != 0) {
                    lb_progress_unlock(0x2F);
                }
                if (lb_unlock_cond_ck(16) != 0) {
                    lb_progress_unlock(0x32);
                }
            }
        }
    }
}

/* Draws the whale-event work's trade seed. */
void lb_kujira_seed_roll(LbKujiraWork* work) {
    work->seed_0x04 = ran_suu(1);
}

/* Opens the trade screen (lobby screen 11): clears its work, rolls the price rate of every goods row from the seed's
 * mood table and picks one to four wanted goods. */
void lb_barter_open(s32 arg_a, s32 arg_b) {
    LbBarterWork* work = lobby_w.menu_0x0AC;
    u16 seed;
    u8* weights;
    u8 i;
    s16 n;
    s16 j;
    s16 count;
    u8 k;
    u16 list[192];

    memset(work, 0, 0x2000);
    work->arg_0x05C = arg_a;
    work->arg_0x060 = arg_b;
    lobby_w.field_0x000 = 11;
    lobby_w.field_0x008 = 1;
    lb_talk_page_open(0);
    seed = lobby_world_block->work_0x4832.seed_0x04;
    weights = lb_barter_rate_weight_tbls[(u8)weight_table_index_u8(lb_barter_mood_weights, 2, seed % 100)];
    for (i = 0; i < 192; i++) {
        seed = rand_lcg_step(seed);
        work->rates_0x074[i] = weight_table_index_u8(weights, 3, seed % 100);
    }
    n = 0;
    for (j = 0; j < 192; j++) {
        if (lb_barter_goods_b[j].item_0x00 == 0) {
            break;
        }
        if (lb_barter_goods_b[j].flag_0x04 != 0) {
            list[n] = lb_barter_goods_b[j].item_0x00;
            n++;
        }
    }
    seed = rand_lcg_step(seed);
    count = seed % 4 + 1;
    for (k = 0; k < count; k++) {
        seed = rand_lcg_step(seed);
        work->wanted_0x134[k] = list[(s16)(seed % n)];
    }
}

/* Closes the trade screen's panel. */
void lb_barter_close(void) {
    lb_panel_close();
}

/* The number of trade kinds the unlock table has opened (ids 9, 12 and 16). */
s16 lb_barter_kind_count(void) {
    s16 n = 0;

    if (lb_unlock_cond_ck(9) != 0) {
        n = 1;
    }
    if (lb_unlock_cond_ck(12) != 0) {
        n++;
    }
    if (lb_unlock_cond_ck(16) != 0) {
        n++;
    }
    return n;
}

/* Whether a trade offer row is open: its unlock condition is accepted, its progress flag (if any) is set, and a
 * once-only row has not been taken yet. */
u32 lb_barter_src_ok_ck(LbBarterSrc* src) {
    if (lb_unlock_cond_ck(src->cond_0x06) == 1 && (src->flag_0x08 == 0xFFFF || userdata_progress_flag_ck(src->flag_0x08) != 0)) {
        if (src->once_0x0A == 1) {
            if (userdata_progress_flag_ck(src->item_0x00) == 0) {
                return 1;
            }
        } else {
            return 1;
        }
    }
    return 0;
}

/* Lists every open offer row of the kinds `mask` selects into the work's reference table. */
void lb_barter_refs_build(LbBarterWork* work, u8* mask) {
    u16 n = 0;
    u16 kind;
    LbBarterSrc* src;
    u16 index;
    u16 end;

    memset(work->refs_0x146, 0xFFFF, 0x300);
    for (kind = 0; kind < 3; kind++) {
        if (*mask & (1 << kind)) {
            if (work->mode_0x010 == 0) {
                work->srcs_0x064[kind] = lb_barter_src_a[kind];
                end = 0;
            } else {
                work->srcs_0x064[kind] = lb_barter_src_b[kind];
                end = 0xFFFF;
            }
            src = work->srcs_0x064[kind];
            for (index = 0; src->item_0x00 != end; index++, src++) {
                if (lb_barter_src_ok_ck(src) == 1) {
                    work->refs_0x146[n].index_0x00 = index;
                    work->refs_0x146[n].kind_0x02 = kind;
                    n++;
                }
            }
        }
    }
}

/* The price rate (percent) of goods row `index`. */
u8 lb_barter_rate_get(LbBarterWork* work, u16 index) {
    return lb_barter_rate_pct[work->rates_0x074[index]];
}

/* Scales `price` by `rate` percent. */
u16 lb_barter_price_scale(u16 price, u8 rate) {
    return price * (rate * 0.01f);
}

/* The price of goods row `index` at its rolled rate. */
u16 lb_barter_row_price_get(LbBarterWork* work, u16 price, u16 index) {
    return lb_barter_price_scale(price, lb_barter_rate_get(work, index));
}

/* Fills the five-row page `page` of the current tab: the offer rows the reference table lists (tab 1) or the stock
 * rows that have goods (tab 0), with their prices at the rolled rates in the selling mode. */
void lb_barter_page_fill(LbBarterWork* work, s16 page) {
    LbBarterEntry* out = work->page_0x448;
    LbBarterRef* ref;
    s32 rows;
    s32 limit;
    s16 first;
    s16 last;
    s16 i;
    LbBarterSrc* srcs;

    if (work->tab_0x030 == 0) {
        rows = 5;
        limit = 64;
    } else {
        rows = 5;
        limit = 192;
        ref = work->refs_0x146;
    }
    first = page * rows;
    last = first + rows - 1;
    memset(out, 0, rows * sizeof(LbBarterEntry));
    i = 0;
    work->shown_0x04C = 0;
    for (;;) {
        if (work->tab_0x030 == 1) {
            if (ref->index_0x00 == 0xFFFF) {
                break;
            }
        } else if (work->stock_0x484[i].price_0x02 == 0) {
            break;
        }
        if (i >= first && i <= last) {
            if (work->tab_0x030 == 1) {
                srcs = work->srcs_0x064[ref->kind_0x02];
                out->item_0x00 = srcs[ref->index_0x00].item_0x00;
                if (work->type_0x000 == 2) {
                    out->count_0x04 = 1;
                    out->rate_0x08 = lb_barter_rate_get(work, ref->index_0x00);
                    out->price_0x02 = lb_barter_row_price_get(work, srcs[ref->index_0x00].price_0x04, ref->index_0x00);
                    out->flag_0x09 = 0;
                } else {
                    out->count_0x04 = srcs[ref->index_0x00].count_0x02;
                    out->rate_0x08 = 0;
                    out->price_0x02 = srcs[ref->index_0x00].price_0x04;
                    out->flag_0x09 = srcs[ref->index_0x00].once_0x0A;
                }
            } else {
                lb_barter_entry_copy(out, &work->stock_0x484[i]);
            }
            work->shown_0x04C++;
            out++;
        }
        i++;
        if (i >= limit) {
            break;
        }
        if (work->tab_0x030 == 1) {
            ref++;
        }
    }
    work->page_0x03C = page;
    work->pages_0x03E = menu_page_count(i, rows);
    if (work->pages_0x03E == 0) {
        work->pages_0x03E = 1;
    }
    work->rows_0x034 = work->shown_0x04C + 1;
    if (work->cursor_0x032 >= work->shown_0x04C) {
        work->cursor_0x032 = work->shown_0x04C - 1;
    }
}

/* Copies a trade row. */
void lb_barter_entry_copy(LbBarterEntry* dst, LbBarterEntry* src) {
    dst->item_0x00 = src->item_0x00;
    dst->price_0x02 = src->price_0x02;
    dst->count_0x04 = src->count_0x04;
    dst->rate_0x08 = src->rate_0x08;
    dst->flag_0x09 = src->flag_0x09;
}

/* Adds `delta` to the stock count of `item`, and on the stock tab to the matching rows of the page. */
void lb_barter_stock_add(LbBarterWork* work, u16 item, s32 delta) {
    s32 i;
    LbBarterEntry* stock;
    LbBarterEntry* page;
    s32 j;

    for (i = 0; i < 64; i++) {
        if (item == work->stock_0x484[i].item_0x00) {
            stock = &work->stock_0x484[i];
            stock->count_0x04 += delta;
            if (work->tab_0x030 != 0) {
                return;
            }
            if (work->page_0x448[0].item_0x00 == stock->item_0x00) {
                work->page_0x448[0].count_0x04 = stock->count_0x04;
            }
            page = &work->page_0x448[1];
            if (page[0].item_0x00 == stock->item_0x00) {
                page[0].count_0x04 = stock->count_0x04;
            }
            if (page[1].item_0x00 == stock->item_0x00) {
                page[1].count_0x04 = stock->count_0x04;
            }
            if (page[2].item_0x00 == stock->item_0x00) {
                page[2].count_0x04 = stock->count_0x04;
            }
            if (page[3].item_0x00 == stock->item_0x00) {
                page[3].count_0x04 = stock->count_0x04;
            }
            return;
        }
    }
}

/* The stock count of `item`, 0 when it is not stocked. */
s32 lb_barter_stock_get(LbBarterWork* work, u16 item) {
    s32 i;

    for (i = 0; i < 64; i++) {
        if (item == work->stock_0x484[i].item_0x00) {
            return work->stock_0x484[i].count_0x04;
        }
    }
    return 0;
}

/* Whether `item` is one of the goods the trader wants. */
s32 lb_barter_wanted_ck(LbBarterWork* work, u16 item) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (item == work->wanted_0x134[i]) {
            return 1;
        }
    }
    return 0;
}

/* Fills the stock rows with the goods the player owns: in the selling mode a wanted good fetches 150 % of its price,
 * the others 100 %. */
void lb_barter_stock_build(LbBarterWork* work) {
    LbBarterEntry* stock;
    LbBarterGoods* goods;
    s16 i;
    s32 count;

    memset(work->stock_0x484, 0, sizeof(work->stock_0x484));
    i = 0;
    goods = (work->type_0x000 == 2) ? lb_barter_goods_b : lb_barter_goods_a;
    stock = work->stock_0x484;
    while (goods->item_0x00 != 0) {
        count = userdata_item_count_total(goods->item_0x00, (u8*)lobby_world_block);
        if (count != 0) {
            stock->item_0x00 = goods->item_0x00;
            stock->count_0x04 = count;
            if (work->type_0x000 == 2) {
                if (lb_barter_wanted_ck(work, goods->item_0x00) != 0) {
                    stock->rate_0x08 = 150;
                } else {
                    stock->rate_0x08 = 100;
                }
                stock->price_0x02 = lb_barter_price_scale(goods->price_0x02, stock->rate_0x08);
            } else {
                stock->price_0x02 = goods->price_0x02;
            }
            stock++;
        }
        goods++;
        i++;
        if (i >= 64) {
            break;
        }
    }
}

/* Empties both carts, rebuilds the stock and shows its first page. */
void lb_barter_reset(LbBarterWork* work) {
    work->page_0x03C = 0;
    work->cursor_0x032 = 0;
    work->cart_row_0x038 = 0;
    work->cart_col_0x03A = 0;
    memset(work->get_0x784, 0, sizeof(work->get_0x784));
    memset(work->pay_0x7D4, 0, sizeof(work->pay_0x7D4));
    lb_barter_stock_build(work);
    lb_barter_page_fill(work, 0);
}

/* Whether the trade can be settled: something is taken and the payment covers it. */
u32 lb_barter_settle_ok_ck(LbBarterWork* work) {
    u16 get = lb_barter_cart_total(work, work->get_0x784);
    u16 pay = lb_barter_cart_total(work, work->pay_0x7D4);

    if (get != 0 && get <= pay) {
        return 1;
    }
    return 0;
}

/* The cart row `item` goes to: in the buying mode (or for a stackable row) the row that already holds it, otherwise
 * the first empty row; -1 when there is none. */
s16 lb_barter_cart_slot_find(LbBarterWork* work, LbBarterCartRow* cart, u16 item, u8 once) {
    LbBarterCartRow* row;
    s16 i;
    s16 slot = -1;

    if (work->mode_0x010 == 0 || once == 1) {
        row = cart;
        for (i = 0; i < 10; i++, row++) {
            if (row->item_0x00 == item && row->count_0x02 != 0) {
                if (once == 0) {
                    slot = i;
                }
                return slot;
            }
        }
    }
    for (i = 0; i < 10; i++) {
        if (cart[i].price_0x04 == 0) {
            slot = i;
            break;
        }
    }
    return slot;
}

/* The cart's total: price times count in the buying mode, the price sum otherwise. */
u16 lb_barter_cart_total(LbBarterWork* work, LbBarterCartRow* cart) {
    u16 total = 0;
    s16 i;

    for (i = 0; i < 10; i++) {
        if (cart[i].price_0x04 != 0) {
            total += (u16)(cart[i].price_0x04 * (work->mode_0x010 == 0 ? cart[i].count_0x02 : 1));
        }
    }
    return total;
}

/* Why page row `index` cannot go into `cart`: 0x10 an empty row, 4 out of stock, 2 over the limit, 1 no cart row, 8 a
 * once-only row already taken, 0x20 a full stack. */
u32 lb_barter_entry_flags(LbBarterWork* work, LbBarterCartRow* cart, s16 index) {
    u32 flags = 0;
    u16 total;
    s16 limit;
    s16 slot;

    if (work->page_0x448[index].price_0x02 == 0) {
        return 0x10;
    }
    if (work->tab_0x030 == 0 && lb_barter_stock_get(work, work->page_0x448[index].item_0x00) == 0) {
        flags |= 4;
    }
    if (work->mode_0x010 == 1 && work->tab_0x030 == 0 && work->pay_0x7D4[0].count_0x02 != 0) {
        flags |= 2;
    }
    total = lb_barter_cart_total(work, cart);
    if (work->mode_0x010 == 0) {
        limit = 1000;
    } else if (work->tab_0x030 == 0) {
        limit = 5;
    } else {
        limit = lb_barter_cart_total(work, work->pay_0x7D4);
    }
    if (work->tab_0x030 == 0) {
        if (total >= limit) {
            flags |= 2;
        }
    } else if (total + work->page_0x448[index].price_0x02 > limit) {
        flags |= 2;
    }
    slot = lb_barter_cart_slot_find(work, cart, work->page_0x448[index].item_0x00, work->page_0x448[index].flag_0x09);
    if (slot == -1) {
        flags |= 1;
    } else if (cart[slot].flag_0x06 == 1) {
        if (userdata_progress_flag_ck(cart[slot].item_0x00) == 1) {
            flags |= 8;
        }
        if (cart[slot].price_0x04 != 0) {
            flags |= 1;
        }
    } else if (cart[slot].count_0x02 >= 99) {
        flags |= 0x20;
    }
    return flags;
}

/* Puts `count` of page row `entry` into `cart`, taking them off the stock on the stock tab. */
void lb_barter_cart_add(LbBarterWork* work, LbBarterCartRow* cart, LbBarterEntry* entry, s16 count) {
    s16 slot = lb_barter_cart_slot_find(work, cart, entry->item_0x00, entry->flag_0x09);
    LbBarterCartRow* row;

    if (slot != -1) {
        row = &cart[slot];
        if (row->price_0x04 == 0) {
            row->item_0x00 = entry->item_0x00;
            row->price_0x04 = entry->price_0x02;
            row->flag_0x06 = entry->flag_0x09;
        }
        row->count_0x02 += (u16)count;
        if (work->tab_0x030 == 0) {
            lb_barter_stock_add(work, row->item_0x00, -count);
        }
    }
}

/* Settles the trade: hands the paid goods over, unlocks the progress entries the taken once-only rows stand for and
 * opens the item hand-over window on the rest; returns how many goods it hands over. */
s16 lb_barter_settle(LbBarterWork* work) {
    s32 i;
    s16 n;

    for (i = 0; i < 10; i++) {
        if (work->pay_0x7D4[i].item_0x00 != 0) {
            eft052_page_count_add(work->pay_0x7D4[i].item_0x00, work->pay_0x7D4[i].count_0x02);
        }
    }
    memset(work->req_0x824, 0, 0x29);
    memset(work->got_0x13C, 0xFFFF, sizeof(work->got_0x13C));
    n = 0;
    work->shown_0x04C = 0;
    for (i = 0; i < 10; i++) {
        if (work->get_0x784[i].flag_0x06 == 1) {
            work->got_0x13C[work->shown_0x04C] = work->get_0x784[i].item_0x00;
            lb_progress_unlock(work->get_0x784[i].item_0x00);
            work->shown_0x04C++;
        } else if (work->get_0x784[i].item_0x00 != 0) {
            work->req_0x824[n].item_0x00 = work->get_0x784[i].item_0x00;
            work->req_0x824[n].count_0x02 = (s16)work->get_0x784[i].count_0x02;
            n++;
        }
    }
    if (n != 0) {
        eft052_item_get_open((Eft052ItemGetReq*)work->req_0x824, lb_item_get_data + 0x60);
    }
    return n;
}

/* Restarts the trade screen on its top step with the current kinds. */
void lb_barter_restart(LbBarterWork* work, u8* mask) {
    work->step_0x001 = 0;
    work->ready_0x058 = 0;
    lb_barter_refs_build(work, mask);
    lb_barter_reset(work);
}

/* One frame of the trade screen: the tab choice (0), picking page rows into a cart (1), taking rows back out of the
 * cart (2), the settle confirmation (3), the hand-over (6, then 4) and the leave confirmation (5).  Returns 1 once the
 * trade is done and 2 when the player leaves. */
s32 lb_barter_step(LbBarterWork* work, u8* mask) {
    s32 result = 0;
    s32 sel = work->cart_row_0x038 + work->cart_col_0x03A * 5;
    u8 ready;
    LbBarterCartRow* cart;

    work->frames_0x054++;
    if (work->blink_0x070 != 0) {
        if (++work->blink_0x070 > 6) {
            work->blink_0x070 = 0;
        }
    }
    ready = work->ready_0x058;
    switch (work->step_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (work->tab_0x030) {
            case 0:
            case 1:
                lb_barter_page_fill(work, 0);
                work->step_0x001 = 1;
                work->cart_row_0x038 = 0;
                work->cart_col_0x03A = 0;
                work->repeat_0x002 = 0;
                if (work->shown_0x04C == 0) {
                    work->cursor_0x032 = work->rows_0x034 - 1;
                } else {
                    work->cursor_0x032 = 0;
                }
                sysSE_req(5);
                break;
            case 2:
                if (lb_barter_settle_ok_ck(work) != 0) {
                    work->step_0x001 = 3;
                    work->dialog_0x050 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (lb_barter_cart_total(work, work->pay_0x7D4) == 0 && lb_barter_cart_total(work, work->get_0x784) == 0) {
                result = 2;
                sysSE_req(1);
            } else {
                work->step_0x001 = 5;
                work->dialog_0x050 = 0;
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->tab_0x030 = menu_cursor_step(work->tab_0x030, 3 - (lb_barter_settle_ok_ck(work) == 0),
                                               lb_cmd_repeat_get(), 1, 2);
            lb_barter_refs_build(work, mask);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0 || (work->repeat_0x002 != 0 && lb_cmd_repeat_ck(0x10) != 0)) {
            if (work->cursor_0x032 == work->rows_0x034 - 1) {
                work->step_0x001 = 0;
                sysSE_req(0);
            } else {
                if (work->tab_0x030 == 1) {
                    if (lb_barter_entry_flags(work, work->get_0x784, work->cursor_0x032) == 0) {
                        if (work->page_0x448[work->cursor_0x032].flag_0x09 == 0 && work->mode_0x010 == 1) {
                            lb_barter_cart_add(work, work->get_0x784, &work->page_0x448[work->cursor_0x032],
                                              (s16)work->page_0x448[work->cursor_0x032].count_0x04);
                        } else {
                            lb_barter_cart_add(work, work->get_0x784, &work->page_0x448[work->cursor_0x032], 1);
                        }
                        work->blink_0x070 = 1;
                        sysSE_stop(16);
                    } else {
                        sysSE_req(2);
                    }
                } else if (lb_barter_entry_flags(work, work->pay_0x7D4, work->cursor_0x032) == 0) {
                    lb_barter_cart_add(work, work->pay_0x7D4, &work->page_0x448[work->cursor_0x032], 1);
                    work->blink_0x070 = 1;
                    sysSE_stop(16);
                } else {
                    sysSE_req(2);
                }
                work->ready_0x058 = lb_barter_settle_ok_ck(work);
                if (ready == 0 && work->ready_0x058 == 1) {
                    sysSE_stop(25);
                } else if (ready == 1 && work->ready_0x058 == 0) {
                    sysSE_stop(26);
                }
                work->repeat_0x002 = 1;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            work->step_0x001 = 0;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x80) != 0) {
            if (work->cursor_0x032 != work->rows_0x034 - 1) {
                if (work->tab_0x030 == 0) {
                    if (lb_barter_cart_total(work, work->pay_0x7D4) != 0) {
                        work->step_0x001 = 2;
                        sysSE_stop(15);
                        work->repeat_0x002 = 0;
                    }
                } else if (lb_barter_cart_total(work, work->get_0x784) != 0) {
                    work->step_0x001 = 2;
                    sysSE_stop(15);
                    work->repeat_0x002 = 0;
                }
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cursor_0x032 = menu_cursor_step(work->cursor_0x032, work->rows_0x034, lb_cmd_repeat_get(), 1, 2);
            work->repeat_0x002 = 0;
        } else if (lb_cmd_repeat_ck(12) != 0 && work->cursor_0x032 < work->rows_0x034 - 1) {
            work->page_0x03C = menu_cursor_step_fixed_tail(work->page_0x03C, work->pages_0x03E, lb_cmd_repeat_get(), 4, 8,
                                                           &work->moved_0x044);
            lb_barter_page_fill(work, work->page_0x03C);
            work->repeat_0x002 = 0;
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(0x10) != 0 || (work->repeat_0x002 != 0 && lb_cmd_repeat_ck(0x10) != 0)) {
            if (work->tab_0x030 == 1) {
                cart = work->get_0x784;
            } else {
                cart = work->pay_0x7D4;
            }
            if (cart[sel].count_0x02 != 0) {
                if (work->mode_0x010 == 1) {
                    cart[sel].count_0x02 = 0;
                } else {
                    cart[sel].count_0x02--;
                }
                if (work->tab_0x030 == 0) {
                    lb_barter_stock_add(work, cart[sel].item_0x00, 1);
                }
                if (cart[sel].count_0x02 == 0) {
                    memset(&cart[sel], 0, sizeof(LbBarterCartRow));
                    work->repeat_0x002 = 0;
                }
                work->blink_0x070 = 1;
                sysSE_stop(17);
            } else {
                sysSE_req(2);
            }
            work->ready_0x058 = lb_barter_settle_ok_ck(work);
            if (ready == 0 && work->ready_0x058 == 1) {
                sysSE_stop(25);
            } else if (ready == 1 && work->ready_0x058 == 0) {
                sysSE_stop(26);
            }
            work->repeat_0x002 = 1;
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            work->step_0x001 = 1;
            work->repeat_0x002 = 0;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x80) != 0) {
            work->step_0x001 = 1;
            work->repeat_0x002 = 0;
            sysSE_stop(15);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            if (work->mode_0x010 == 0 || work->tab_0x030 == 1) {
                work->cart_row_0x038 = menu_cursor_step(work->cart_row_0x038, 5, lb_cmd_repeat_get(), 4, 8);
                work->repeat_0x002 = 0;
            }
        } else if (lb_cmd_repeat_ck(3) != 0 && work->mode_0x010 == 0) {
            work->cart_col_0x03A = menu_cursor_step(work->cart_col_0x03A, 2, lb_cmd_repeat_get(), 1, 2);
            work->repeat_0x002 = 0;
        }
        break;
    case 3:
        switch (((s32 (*)(u32*, u16, s32, s32, u16))toggle_word_step)(&work->dialog_0x050, lobby_cmd_trig_get(), 4, 8,
                                                                    0xFFFF)) {
        case 1:
            work->count_0x046 = lb_barter_settle(work);
            if (work->shown_0x04C != 0) {
                work->step_0x001 = 6;
                work->got_index_0x007 = 0;
                sysSE_stop(21);
            } else {
                work->step_0x001 = 4;
                sysSE_stop(21);
            }
            break;
        case 2:
            work->step_0x001 = 0;
            break;
        }
        break;
    case 5:
        switch (lb_yes_no_step(&work->dialog_0x050)) {
        case 1:
            result = 2;
            break;
        case 2:
            work->step_0x001 = 0;
            break;
        }
        break;
    case 6:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            work->blink_0x070 = 1;
            if (++work->got_index_0x007 >= work->shown_0x04C) {
                if (work->count_0x046 != 0) {
                    work->step_0x001 = 4;
                    sysSE_stop(21);
                } else {
                    result = 1;
                    sysSE_req(0);
                }
            } else {
                sysSE_req(27);
            }
        }
        break;
    case 4:
        switch (eft052_item_get_step()) {
        case 1:
        case 2:
            result = 1;
            break;
        }
        break;
    }
    return result;
}

/* Seeds the trade screen's top menu: one row per opened trade kind, then rows 3 and 4; takes the visit's kind bits
 * and the item the trader was given. */
void lb_barter_menu_init(LbBarterWork* work) {
    LbBlockView* block = lobby_world_block;
    u8 i;

    work->cursor_0x032 = 0;
    work->field_0x006 = 0;
    memset(work->menu_0x009, 0, sizeof(work->menu_0x009));
    work->menu_count_0x036 = lb_barter_kind_count();
    work->rows_0x034 = 0;
    for (i = 0; i < work->menu_count_0x036; i++) {
        work->menu_0x009[work->rows_0x034] = i;
        work->rows_0x034++;
    }
    work->menu_0x009[work->rows_0x034] = 3;
    work->rows_0x034++;
    work->menu_0x009[work->rows_0x034] = 4;
    work->rows_0x034++;
    work->ship_bits_0x003 = block->work_0x4832.bits_0x01;
    work->item_0x004 = block->work_0x4832.item_0x06;
    if (work->item_0x004 >= 747) {
        work->item_0x004 = 0;
    }
}

/* The item-hold strip entry the trade screen hands `eft052_hold_entry_set` (`ef/eft052.cpp`'s `CockpitHoldEntry`
 * layout).  size: 0x1C */
typedef struct LbBarterHoldEntry {
    /* +0x00 */ s32 limit_0x00;
    /* +0x04 */ s16 msg_0x04;
    /* +0x06 */ s16 msg2_0x06;
    /* +0x08 */ s16 title_0x08;
    /* +0x0A */ s16 count_msg_0x0A;
    /* +0x0C */ s16 item_count_0x0C;
    /* +0x0E */ u16 sprite_0x0E;
    /* +0x10 */ u16 highlight_0x10;
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u16* items_0x14;
    /* +0x18 */ u8 kind_0x18;
    /* +0x19 */ u8 pad_0x19[3];
} LbBarterHoldEntry;

/* Opens the item hold strip on the trader's gift items the player owns. */
void lb_barter_hold_open(LbBarterWork* work) {
    LbBarterHoldEntry entry;
    s32 i;

    memset(work->hold_items_0x850, 0, sizeof(work->hold_items_0x850));
    work->count_0x046 = 0;
    for (i = 0; i < 2; i++) {
        if (userdata_item_count_total(lb_kujira_gift_items[i], (u8*)lobby_world_block) != 0) {
            work->hold_items_0x850[work->count_0x046] = lb_kujira_gift_items[i];
            work->count_0x046++;
        }
    }
    entry.limit_0x00 = 0;
    entry.msg_0x04 = 342;
    entry.msg2_0x06 = 348;
    entry.count_msg_0x0A = -1;
    entry.title_0x08 = 343;
    entry.sprite_0x0E = 5093;
    entry.highlight_0x10 = work->item_0x004;
    entry.item_count_0x0C = work->count_0x046;
    entry.items_0x14 = work->hold_items_0x850;
    entry.kind_0x18 = 0;
    eft052_hold_entry_set(&entry, 0);
}

/* Lists the open offer rows of kind `kind` from both tables: the first table's rows from 0, the second's from the
 * first page after them, and counts the pages of each. */
void lb_barter_refs_kind_build(LbBarterWork* work, u8 kind) {
    u16 n = 0;
    LbBarterSrc* src;
    u16 index;
    u16 m;
    u16 index2;

    memset(work->refs_0x146, 0xFFFF, 0x300);
    work->kind_0x008 = kind;
    src = lb_barter_src_a[kind];
    for (index = 0; src->item_0x00 != 0; index++, src++) {
        if (lb_barter_src_ok_ck(src) == 1) {
            work->refs_0x146[n].index_0x00 = index;
            work->refs_0x146[n].kind_0x02 = kind;
            n++;
        }
    }
    work->pages_a_0x040 = menu_page_count(n, 5);
    src = lb_barter_src_b[kind];
    index2 = 0;
    for (m = work->pages_a_0x040 * 5; src->item_0x00 != 0xFFFF; index2++, src++) {
        if (lb_barter_src_ok_ck(src) == 1) {
            work->refs_0x146[m].index_0x00 = index2;
            work->refs_0x146[m].kind_0x02 = kind;
            m++;
        }
    }
    work->pages_0x03E = menu_page_count(m, 5);
}

/* Fills the five-row page `page` of the current kind's offer list: rows of the first table are priced at the base
 * rate, rows of the second carry their own count, price and once flag. */
void lb_barter_page_fill_kind(LbBarterWork* work, s16 page) {
    LbBarterEntry* out = work->page_0x448;
    LbBarterRef* ref;
    LbBarterSrc* srcs;
    s16 i;

    memset(out, 0, 5 * sizeof(LbBarterEntry));
    work->shown_0x04C = 0;
    ref = &work->refs_0x146[(s16)(page * 5)];
    if (page < work->pages_a_0x040) {
        srcs = lb_barter_src_a[work->kind_0x008];
    } else {
        srcs = lb_barter_src_b[work->kind_0x008];
    }
    for (i = 0; i < 5; i++, ref++, out++) {
        if (ref->index_0x00 == 0xFFFF) {
            break;
        }
        out->item_0x00 = srcs[ref->index_0x00].item_0x00;
        if (page < work->pages_a_0x040) {
            out->count_0x04 = 1;
            out->rate_0x08 = lb_barter_rate_pct[1];
            out->price_0x02 = srcs[ref->index_0x00].price_0x04;
            out->flag_0x09 = 0;
        } else {
            out->count_0x04 = srcs[ref->index_0x00].count_0x02;
            out->rate_0x08 = 0;
            out->price_0x02 = srcs[ref->index_0x00].price_0x04;
            out->flag_0x09 = srcs[ref->index_0x00].once_0x0A;
        }
        work->shown_0x04C++;
    }
    work->page_0x03C = page;
    if (work->cursor_b_0x042 >= work->shown_0x04C) {
        work->cursor_b_0x042 = work->shown_0x04C - 1;
    }
}

} /* extern "C" */
