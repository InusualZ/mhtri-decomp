/* ef/eft050_set.cpp - the eft050_set record: the spawner, its release and dispatch hooks, its create and move states
 *   and the two shared state helpers its dispatch calls.
 * RANGE. .text 0x80342F34-0x80343758 (7 functions, in address order); extab 0x80016D9C-0x80016DBC, extabindex
 *   0x800362D0-0x80036300, .data 0x805E7B50-0x805E7B70 (the spin steps and the scales), .sdata2 0x8079B318-0x8079B350.
 *   Cut from `ef/eft050.cpp`: the `.sdata2` pool repeats 1.0f, 0.0f and both int-to-float magics of ef/eft050's pool,
 *   and the extabindex record at 0x800362D0 pins the left edge.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `optimization_level 4` and `fp_contract off` for the file.
 * NAMES. `eft050_set` is the runtime dump's name.  GUESS (from each body, its callers and its data): `eft050_set_release`/
 *   `_dispatch`/`_create`/`_move`, `eft050_set_spin_steps` (the four per-model Z spin steps `eft050_set_move` adds each
 *   frame), `eft050_set_scales`, `eft_state_advance`, `eft_instance_release`.
 * RESIDUALS. none: `.text`, `extab`/`extabindex`, `.data` (the spin steps then the scales) and `.sdata2` are
 *   byte-identical to the target object.
 * SHAPES. `eft050_set_move`: `&eft_control` held in a local taken before the switch, the 20.0f camera distance in a
 *   local set before `get_camera_pos`, the first alpha in a `u8` local set on both arms, `i` declared first.
 *   `dist` is declared `= 0.0f`: the dead initialiser puts 0.0f in the pool ahead of 20.0f, retail's `.sdata2` order
 *   (without it 0.0f lands after 2.125f, where the loop first stores it).
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
#include "ef/eft050_set.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The Z spin step `eft050_set_move` adds to each of the four models every frame. */
s32 eft050_set_spin_steps[4] = { 46, 0xFFC4, 76, 0xFFA6 };

/* The uniform scale `eft050_set_create` gives each of the four models. */
f32 eft050_set_scales[4] = { 14.0f, 12.0f, 9.0f, 5.0f };

#ifdef __cplusplus
}
#endif

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

/* Pools the `eft050` record: the effect, its work block, four pooled model handles, the extra record
 * the fourth pool call hands back, and the position/source/area seeds. */
void eft050_set(_PLW* plw, nw4r::math::VEC3* pos, u8 flag)
{
    s32 i;
    _EFT* eft;
    Eft050SetWork* work;

    eft = (_EFT*)eft_res_slot_get(0x2C);
    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = 0;
    eft->dispatch_0x34 = eft050_set_dispatch;
    eft->release_0x40 = eft050_set_release;
    work = (Eft050SetWork*)eft->work_0x38;
    work->count_0x00 = 4;
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    work->extra_0x14 = eft_res_model_get();
    if (work->extra_0x14 == NULL) {
        eft_res_slot_release(eft);
        return;
    }
    memset(work->works_0x1C, 0, 8);
    eft->field_0x03 = 0x32;
    eft_state_flags_set(eft, 1, 0);
    copyVec3(&eft->pos_0x18, pos);
    eft->area_0x44 = flag;
    eft->source_0x30 = plw;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
}

/* Releases the `eft050` family's pools: every model handle one at a time, the two `_g3d_work`
 * handles in reverse order, then the record the create filled. */
extern "C" void eft050_set_release(_EFT* self)
{
    s32 i;
    Eft050SetWork* work = (Eft050SetWork*)self->work_0x38;

    for (i = 0; i < work->count_0x00; i++) {
        eft_res_models_release(&work->models_0x04[i], 1);
    }
    for (i = 1; i >= 0; i--) {
        if (work->works_0x1C[i] != NULL) {
            push_g3d_wk(work->works_0x1C[i]);
        }
    }
    eft_res_models_release(&work->extra_0x14, 1);
}

/* The `eft050` family's own state machine (its three bodies sit in the band above this one). */
extern "C" void eft050_set_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return eft050_set_create(self);
    case 1: return eft050_set_move(self);
    case 2: return eft_state_advance(self);
    case 3: return eft_instance_release(self);
    }
}

/* Builds the eft050_set record's models: the UV model of the pool and the four plain models with their scales and a
 * random roll. */
extern "C" void eft050_set_create(_EFT* self)
{
    Eft050SetWork* work = (Eft050SetWork*)self->work_0x38;
    u32 i;

    self->state_0x05++;
    work->created_0x18 = res_eft_UV_model_create((MHchar*)work->extra_0x14, 73, 332, 0, work->works_0x1C, 1, 0);
    if (work->created_0x18 == NULL) {
        eft_instance_release(self);
        return;
    }
    for (i = 0; i < (u32)work->count_0x00; i++) {
        if (res_eft_model_create(work->models_0x04[i], 74, 76) == NULL) {
            eft_instance_release(self);
            return;
        }
    }
    work->scale_0x28 = 1.0f;
    setVector3(&((MHchar*)work->extra_0x14)->scale_0x1C, 1.5f, 1.5f, 1.5f);
    ((MHchar*)work->extra_0x14)->field_0x28 = 0;
    ((MHchar*)work->extra_0x14)->field_0x2C = 0;
    ((MHchar*)work->extra_0x14)->field_0x30 = 0;
    ((MHchar*)work->extra_0x14)->ready = 3;
    for (i = 0; i < (u32)work->count_0x00; i++) {
        setVector3(&work->models_0x04[i]->scale_0x1C, eft050_set_scales[i], eft050_set_scales[i], eft050_set_scales[i]);
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->field_0x2C = 0;
        work->models_0x04[i]->field_0x30 = ran_suu(0);
        work->models_0x04[i]->ready = 1;
    }
    eft050_set_move(self);
}

/* Steps the eft050_set fade, then places the UV model in front of a near camera, or spreads and spins the four
 * models along the view axis. */
extern "C" void eft050_set_move(_EFT* self)
{
    VEC3 ofs;
    VEC3 cam;
    MTX34 view;
    MTX34 mtx;
    VEC3 dir;
    u32 i;
    Eft050SetWork* work;
    EftControl* control;
    s32 fade;
    f32 dist = 0.0f;
    u8 alpha;

    work = (Eft050SetWork*)self->work_0x38;
    VEC3_ctor(&ofs);
    VEC3_ctor(&cam);
    MTX34_ctor(&view);
    MTX34_ctor(&mtx);
    VEC3_ctor(&dir);
    control = &eft_control;
    switch (self->field_0x06) {
    case 0:
        fade = 0;
        if (++self->timer_0x0C >= 120) {
            self->field_0x06++;
            self->timer_0x0C = 0;
            fade = 1;
        }
        break;
    case 1:
        fade = 1;
        if (++self->timer_0x0C >= 450) {
            self->field_0x06++;
            self->timer_0x0C = 120;
            fade = 0;
        }
        break;
    case 2:
        fade = 0;
        if (--self->timer_0x0C <= 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
        }
        break;
    }
    if (self->area_0x44 != get_now_areano()) {
        return;
    }
    if (camera_underwater_ck() == 1) {
        return;
    }
    copyVec3(&dir, &get_camera_direction());
    vec3_normalize_into(&dir, &dir);
    dist = 20.0f;
    copyVec3(&cam, &get_camera_pos());
    copyVec3(&((MHchar*)work->extra_0x14)->pos_0x04, &VEC3_ADD(&cam, &VEC3_SCALE(&dir, dist)));
    ((MHchar*)work->extra_0x14)->field_0x28 = camera_angle_x_get();
    ((MHchar*)work->extra_0x14)->field_0x2C = camera_angle_y_get();
    ((MHchar*)work->extra_0x14)->field_0x30 = 0;
    if (hit_point_sphr(&cam, &self->pos_0x18, 1900.0f) == 1) {
        if (control->frame_ready_0xC40 == 0) {
            ((MHchar*)work->extra_0x14)->move(0);
            ((EftUvModelObject*)work->created_0x18)->update();
            if (fade == 0) {
                alpha = 2.125f * self->timer_0x0C;
            } else {
                alpha = 255;
            }
            mhchar_mat_alpha_set((MHchar*)work->extra_0x14, 4, alpha, 0);
            eft_res_models_spawn(self, &work->extra_0x14, 2, 1, 0);
            control->frame_ready_0xC40 = 1;
        }
    } else {
        mtx34_copy(&view, &get_current_view_mtx());
        mtx34_inverse(&view, &view);
        for (i = 0; i < (u32)work->count_0x00; i++) {
            work->models_0x04[i]->field_0x30 += eft050_set_spin_steps[i];
            ofs.x = 0.0f;
            ofs.y = 0.0f;
            ofs.z = 400.0f * i;
            mulVecMat(&ofs, &view);
            if (fade == 0) {
                work->scale_0x28 = self->timer_0x0C / 150.0f;
            } else {
                work->scale_0x28 = 0.625f;
            }
            mhchar_mat_alpha_set(work->models_0x04[i], 4, (u8)(255.0f * work->scale_0x28), 0);
            copyVec3(&work->models_0x04[i]->pos_0x04, &VEC3_ADD(&self->pos_0x18, &ofs));
            mtx34_identity(&mtx);
            mtx34_set_trans(&mtx, &work->models_0x04[i]->pos_0x04);
            mtx34_concat_assign(&mtx, &view);
            rotLocalMatZ(work->models_0x04[i]->field_0x30, &mtx);
            work->models_0x04[i]->move2(&mtx, 0);
            eft_res_models_spawn(self, (void**)&work->models_0x04[i], 2, 1, 0);
        }
    }
}

/* Advances the family's step counter. */
extern "C" void eft_state_advance(_EFT* self) {
    self->state_0x05++;
}

/* Hands the instance to the shared effect release path. */
extern "C" void eft_instance_release(_EFT* self) {
    eft_res_slot_release(self);
}
