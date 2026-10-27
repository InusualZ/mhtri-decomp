/* ef/eft051.cpp - the eft051 lobby decoration family: the spawner a seated lobby NPC calls, its release and
 *   dispatch hooks and its create/alive/advance/retire states.
 * RANGE. .text 0x80342B70-0x80342F34 (7 functions, in address order); extab 0x80016D84-0x80016D9C, extabindex
 *   0x800362AC-0x800362D0, .data 0x805E7A80-0x805E7B50 (the two spot tables), .sdata2 0x8079B310-0x8079B318 (1.0f and
 *   the pad to 8).  Cut from `ef/eft050.cpp`: the `.sdata2` 1.0f repeats at 0x8079B2EC (ef/eft050) and 0x8079B318
 *   (ef/eft050_set), and the extabindex records at 0x800362AC/0x800362D0 pin both edges.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `optimization_level 4` and `fp_contract off` for the file.
 * NAMES. GUESS (module name from `eft051_spawn` seeding `_EFT::field_0x03 = 49`, `ef/eft050.cpp`'s scheme):
 *   `eft051_spawn`/`_release`/`_dispatch`/`_create`/`_alive`/`_state_next`/`_retire`, `eft051_spots_m21`/`_m22`
 *   (the spot tables of maps 21 and 22).
 * RESIDUALS. none: `.text`, `extab`/`extabindex`, `.data` (the two spot tables, defined in retail order) and `.sdata2`
 *   (the 1.0f and dtk's 4-byte alignment tail) are byte-identical to the target object.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "pl.h"
#include "hud/cockpit_quest.h" /* hud/layout.h + pl.h: the 2D element library and its records */
#include "stage/stg_w.h"     /* get_now_areano (the owner header, rule 2) */
#include "ef/fn_800CDB2C.h" /* push_g3d_wk */
#include "Runtime.PPCEABI.H/memset.h" /* memset - owner Runtime.PPCEABI.H/memset.c */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "nw4r/math_types.h" /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h" /* the vector helpers (rule 2) */
#include "Pl/pl_hit_sphere.h" /* hit_point_sphr (rule 2) */
#include "Pl/fn_80273B14.h" /* fn_802754C0 */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2) */
#include "unsplit/ef_control.h" /* eft_control */
#include "enemy/enemy_control.h" /* fn_80143174 */
#include "sound/fn_800D7F54.h" /* se_req_orb (rule 2) */
#include "Pl/Pl_master_ck.h" /* Pl_master_ck, Pl_act_ck (rule 2) */
#include "Pl/pl_skill.h" /* Pl_Skill_ck (rule 2) */
#include "lobby/lb_screen13_work.h" /* LbScreen13Work (rule 1) */
#include "lobby/lb_s13_help_line_draw.h" /* the screen-13 draw helpers (rule 2) */
#include "lobby/lobby_w.h" /* lobby_w (rule 2) */
#include "hud/cockpit.h" /* menu_money_draw (rule 2) */
#include "hud/draw_lsp_sprite_variant.h" /* draw_lsp_sprite_variant (rule 2) */
#include "sound/set_zmode.h" /* set_zmode (rule 2) */
#include "lobby/lb_s13_status_icon_ids.h" /* lb_s13_status_icon_ids (rule 2) */
#include "camera/camera.h" /* get_camera_pos / get_camera_direction / get_current_view_mtx (rule 2) */
#include "camera/camera_angle_x_get.h" /* camera_angle_x_get / camera_angle_y_get / camera_underwater_ck (rule 2) */
#include "rotLocalMatX.h" /* rotLocalMatZ (rule 2) */
#include "sound/mhchar_mat_alpha_set.h" /* mhchar_mat_alpha_set (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "ef/eft001.h" /* mtx34_set_trans (rule 2) */
#include "ef/eft051.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The decoration spots of map 21 (`eft051_create` reads them in area 2), indexed by the source's spot byte. */
EftSpot eft051_spots_m21[12] = {
    { { -580.0f, 223.0f, -435.0f }, 0x58E4, 0 },
    { { -690.0f, 223.0f, -580.0f }, 0x58E4, 0 },
    { { -555.0f, 223.0f, -645.0f }, 0xDA51, 0 },
    { { -450.0f, 223.0f, -500.0f }, 0xDA51, 0 },
    { { 750.0f, 223.0f, -495.0f }, 0x8AAB, 0 },
    { { 580.0f, 223.0f, -440.0f }, 0x8AAB, 0 },
    { { 530.0f, 223.0f, -570.0f }, 0x0CCD, 0 },
    { { 700.0f, 223.0f, -625.0f }, 0x0CCD, 0 },
    { { -95.0f, 550.0f, -1920.0f }, 0xB3E9, 0 },
    { { -45.0f, 550.0f, -2090.0f }, 0xB3E9, 0 },
    { { -195.0f, 550.0f, -2100.0f }, 0x3555, 0 },
    { { -245.0f, 550.0f, -1935.0f }, 0x3555, 0 },
};

/* The decoration spot of map 22 (`eft051_create` reads it in area 0). */
EftSpot eft051_spots_m22[1] = {
    { { -540.0f, 252.0f, -670.0f }, 0x98E4, 0 },
};

#ifdef __cplusplus
}
#endif

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

/* Spawns the eft051 decoration of a lobby object `src` (kinds 0-2): one pooled model, the source's spot index. */
extern "C" void eft051_spawn(_PLW* src, u8 kind)
{
    _EFT* rec;
    Eft051Work* work;
    s32 i;

    if (kind > 2) {
        return;
    }
    rec = (_EFT*)eft_res_slot_get(0xC);
    if (rec == NULL) {
        return;
    }
    rec->release_0x40 = eft051_release;
    work = (Eft051Work*)rec->work_0x38;
    work->count_0x00 = 1;
    rec->area_0x44 = get_now_areano();
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == NULL) {
            eft_res_slot_release(rec);
            return;
        }
    }
    rec->source_0x30 = NULL;
    rec->dispatch_0x34 = eft051_dispatch;
    rec->field_0x03 = 49;
    eft_state_flags_set(rec, 8, 0);
    work->spot_0x08 = src->field_0x0B6;
    work->kind_0x09 = kind;
}


/* Releases a second family's pooled handle list, from its word behind the count. */
extern "C" void eft051_release(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    eft_res_models_release(work->models_0x04, work->count_0x00);
}

/* The second family's state machine: 0 the model-create pass, 1 the alive body, 2 the state advance
 * and 3 the pool release. */
extern "C" void eft051_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return eft051_create(self);
    case 1: return eft051_alive(self);
    case 2: return eft051_state_next(self);
    case 3: return eft051_retire(self);
    }
}

/* Builds the eft051 models at the spot of the map's table (map 22 area 0, map 21 area 2), each showing the kind's
 * joint; no table ends the record. */
extern "C" void eft051_create(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    EftSpot* spots = NULL;
    s32 i;
    s32 j;
    u32 joints;

    self->state_0x05++;
    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->models_0x04[i], 71, 8) == NULL) {
            eft051_retire(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    switch (get_now_mapno()) {
    case 22:
        if ((s32)self->area_0x44 == 0) {
            spots = eft051_spots_m22;
        }
        break;
    case 21:
        if (self->area_0x44 == 2) {
            spots = eft051_spots_m21;
        }
        break;
    }
    if (spots == NULL) {
        eft051_retire(self);
        return;
    }
    for (i = 0; i < work->count_0x00; i++) {
        vec_to_mh_vec3(&work->models_0x04[i]->pos_0x04, &spots[work->spot_0x08].pos);
        setVector3(&work->models_0x04[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->field_0x2C = spots[work->spot_0x08].yaw;
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->ready = 0;
        joints = work->models_0x04[i]->get_joint_num();
        for (j = 0; j < joints; j++) {
            if (j == work->kind_0x09 + 1) {
                work->models_0x04[i]->setVisibility(j, 1);
            } else {
                work->models_0x04[i]->setVisibility(j, 0);
            }
        }
    }
    eft051_alive(self);
}


/* State 1 of the second family: retires the effect once its timer runs past 0xB4, otherwise moves
 * every pooled model to its idle state and updates it. */
extern "C" void eft051_alive(_EFT* self)
{
    s32 i;
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    if (++work->timer_0x0A > 0xB4) {
        eft051_retire(self);
        return;
    }
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i]->move(0);
        eft_res_models_spawn(self, (void**)&work->models_0x04[i], 2, 1, NULL);
    }
}

/* State 2 of the second family's state machine: advances to the next body. */
extern "C" void eft051_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the second family's state machine: releases the pooled effect record. */
extern "C" void eft051_retire(_EFT* self)
{
    eft_res_slot_release(self);
}
