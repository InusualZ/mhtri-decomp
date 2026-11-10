/*
 * ef/eft034.cpp - the tag-34 enemy effect: an orb the player charges and throws, with a ray model that flickers
 *   while it charges (`fn_802F39DC` seeds `field_0x03 = 34`, `fn_802F3B0C` is its `state_0x05` machine).
 * RANGE. .text 0x802F39DC-0x802F5138 (12 functions, in address order); extab 0x800154D4-0x80015514, extabindex
 *   0x80033DA4-0x80033E04, .data 0x805D6CE8-0x805D6D10 (`eft034_ray_tip_offsets`), .sdata2 0x8079A9F0-0x8079AA88
 *   (its literal pool).  `ef/eft035.cpp` ends at 0x802F39DC: the `.sdata2` pool restarts at 0x8079A9F0 (a second
 *   1.0f), extabindex record 0x80033DA4 is `fn_802F39DC`'s, and no call or data reference crosses the seam.
 * FLAGS. `cflags_main`; `#pragma pool_data off`, `peephole off`, `optimization_level 4`, `fp_contract off` file-wide.
 * NAMES. GUESS: the file is `eft034` from the family tag 34 (each eftNNN unit seeds its own number into
 *   `field_0x03`: eft004 4, eft009 9, eft019 19, eft035 35); `eft034_ray_flicker`, `eft034_ray_origin_get`,
 *   `eft034_throw_start`, `eft034_orb_move`, `eft034_throw_act_ck`, `eft034_ray_tip_offsets` from each body and its
 *   callers.  The map has only `fn_` stems for the rest, so plain definitions are `extern "C"`.
 * RESIDUALS. none.
 * SHAPES. the 76-byte work block `eft_res_slot_get(76)` pools (`Eft034Work`).
 */

#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft019.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "pl.h"
#include "fn_8004CAD8.h"               /* MTX34::MTX34() */
#include "Pl/fn_8028F66C.h"            /* VEC3::operator= */
#include "g3d/g3d_calcworld.h"         /* VEC3::operator+= */
#include "sound/fn_800D7F54.h"         /* em015_denki_eft_se_req, se_req_pos_ps */
#include "stage/stg_w.h"               /* get_now_areano */
#include "Runtime.PPCEABI.H/memset.h"  /* memset - owner Runtime.PPCEABI.H/memset.c */
#include "nw4r/g3d/scnmdl.h"             /* nw4r::g3d::ScnMdl::CopiedMatAccess */
#include "g3d/g3d_scnmdl.h"              /* scn_mdl_mat_access_get_tev_color (rule 2) */
#include "g3d/g3d_resmat.h"              /* nw4r::g3d::ResMatTevColor (rule 2) */
#include "g3d/g3d_calcmaterial.h"        /* res_mat_tev_color_copy_ctor / _end_edit (rule 2) */
#include "unsplit/sound.h"                /* fn_800E2994 */
#include "camera/camera.h"               /* get_camera_pos (rule 2) */
#include "ai/fn_802D44F4.h"              /* player_color_get (rule 2) */
#include "stage/get_now_mapno.h"          /* get_now_mapno (rule 2) */
#include "ef/eft001.h"                    /* mtx34_set_trans (rule 2) */
#include "ef/eft004_fx.h"                 /* fn_801006A0 (rule 2) */
#include "Pl/pl_now_area_ck.h"              /* fn_8026FE98 */
#include "Pl/pl_skill.h"                 /* Pl_Skill_ck (rule 2) */
#include "Pl/fn_802693C4.h"              /* Get_motion_no / Pl_frame_check (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * this unit's own bodies, forward-declared (address order)
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_802F3AD0(_EFT* self);
extern "C" void fn_802F3B0C(_EFT* self);
extern "C" void fn_802F3B48(_EFT* self);
extern "C" void fn_802F3D94(_EFT* self);
extern "C" void fn_802F49C8(_EFT* self);
extern "C" void fn_802F49D8(_EFT* self);
extern "C" void eft034_ray_flicker(_EFT* self);
extern "C" void eft034_ray_origin_get(_EFT* self, nw4r::math::VEC3* out);
extern "C" void eft034_throw_start(_EFT* self, u8 phase);
extern "C" void eft034_orb_move(_EFT* self);
extern "C" u32 eft034_throw_act_ck(_EFT* self);

/* Work block of the tag-34 family `fn_802F39DC` seeds (the 76-byte block `eft_res_slot_get(76)` pools): the orb and
 * ray models, the orb's velocity and throw target, its timers and the floor it lands on. size: 0x4C */
struct Eft034Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[2];  /* the orb, then the ray */
    /* +0x0C */ nw4r::math::VEC3 vel_0x0C;
    /* +0x18 */ nw4r::math::VEC3 target_0x18; /* where the throw carries the orb */
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ s32 timer_0x30;          /* the phase's frame count (the hit count while it hovers) */
    /* +0x34 */ s32 ray_timer_0x34;      /* the ray's frame count */
    /* +0x38 */ f32 gravity_0x38;
    /* +0x3C */ f32 scale_0x3C;          /* the orb's scale */
    /* +0x40 */ u8 scaled_0x40;          /* the throw keeps its full speed */
    /* +0x41 */ u8 phase_0x41;           /* the throw phase, 0..2 */
    /* +0x42 */ u8 ray_on_0x42;          /* the ray shows this frame */
    /* +0x43 */ u8 stage_0x43;           /* the player's charge stage last frame */
    /* +0x44 */ s32 jitter_0x44;         /* frames until the hovering orb jitters */
    /* +0x48 */ f32 floor_0x48;          /* the floor height of the area */
};

/* The player byte the orb reads that `Pl/plw.h` keeps inside a padding run: the charge stage of the orb throw.
 * size: 0x598 - lower bound, an approximation (a view of `_PLW`) */
struct _EFT034_PL_CHARGE {
    /* +0x000 */ u8 pad_0x000[0x597];
    /* +0x597 */ u8 stage_0x597;
};

/* The player's physics block (`_PLW::physics_0x13C`): the joint character sits at +0x04.  size: 0x144 */
struct _EFT034_PL_PHYS {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr;
};

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.data` table
 * ------------------------------------------------------------------------------------------------- */

#pragma pool_data off /* the table is reached by name, never through a .data anchor (playbook 194) */

extern "C" {

/* the tag-34 ray's tip offsets per swing pose */
Vec eft034_ray_tip_offsets[3] = {
    { 0.0f, -10.0f, 167.0f },
    { 0.0f, -70.0f, 135.0f },
    { 0.0f, -97.0f, 72.0f },
};

}

/* The by-value spelling of the out-pointer vector subtraction (the result pointer is the hidden first argument). */

/* `eft004_key_spawn` (`ef/eft004_fx.cpp`'s ground burst) with its scale third: every caller loads f1 between r4 and r5. */
#define EFT_GROUND_BURST ((void (*)(u8, nw4r::math::VEC3*, f32, u32, u8, u8))eft004_key_spawn)

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

/* Spawns the tag-34 effect: pools two model handles, seeds the family's zeroed position/rotation and
 * clock, then installs the two hooks that travel with the record. */
extern "C" void fn_802F39DC(_ENEMY_WORK* self, u8 type)
{
    _EFT* eft;
    s32 i;
    Eft034Work* work;

    eft = (_EFT*)eft_res_slot_get(76);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = fn_802F3AD0;
    work = (Eft034Work*)eft->work_0x38;
    eft->type_0x02 = type;
    work->count_0x00 = 2;
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
    eft_state_flags_set(eft, 1, 0);
    eft->field_0x03 = 34;
    eft->area_0x44 = self->field_0x016;
    eft->flag_0x01 = 1;
    eft->source_0x30 = self;
    eft->dispatch_0x34 = fn_802F3B0C;
}

/* Releases the tag-34 family's pooled model handles and clears the pool length. */
extern "C" void fn_802F3AD0(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;

    eft_res_models_release(&work->models_0x04[0], work->count_0x00);
    work->count_0x00 = 0;
}

/* The tag-34 record's per-frame dispatcher: `state_0x05` 0 builds, 1 runs, 2 advances and 3 retires
 * the record. */
extern "C" void fn_802F3B0C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_802F3B48(self);
        break;
    case 1:
        fn_802F3D94(self);
        break;
    case 2:
        fn_802F49C8(self);
        break;
    case 3:
        fn_802F49D8(self);
        break;
    }
}

/* Builds the tag-34 effect: the orb model (its sub-model by the player's act, the player's colour), the ray model, a
 * random launch velocity along the player's facing, the orb's scale and the floor height of the map. */
extern "C" void fn_802F3B48(_EFT* self)
{
    s32 i;
    _PLW* plw = (_PLW*)self->source_0x30;
    Eft034Work* work = (Eft034Work*)self->work_0x38;
    _GXColor col;
    u32 c;

    self->state_0x05++;
    if (res_eft_model_create(work->models_0x04[0], 50, 24) == 0) {
        fn_802F49D8(self);
        return;
    }
    work->models_0x04[0]->ready = 0;
    if (plw->field_0x306 == 206) {
        work->models_0x04[0]->setVisibility(2, 1);
        work->models_0x04[0]->setVisibility(1, 0);
        self->type_0x02 = 1;
    } else {
        work->models_0x04[0]->setVisibility(2, 0);
        work->models_0x04[0]->setVisibility(1, 1);
    }
    c = player_color_get(plw->chunk_ofs);
    col.r = c >> 24;
    col.g = (c >> 16) & 0xFF;
    col.b = (c >> 8) & 0xFF;
    col.a = c;
    for (i = 0; i < 2; i++) {
        work->models_0x04[0]->setTevKColor(i, (_GXTevKColorID)3, &col);
    }
    if (res_eft_model_create(work->models_0x04[1], 49, 16) == 0) {
        fn_802F49D8(self);
        return;
    }
    setVector3(&work->models_0x04[1]->scale_0x1C, 1.0f, 1.0f, 1.0f);
    ((_EFT034_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
    self->rot_0x24.y = plw->field_0x058;
    work->vel_0x0C.x = eft_rand_signed_half();
    work->vel_0x0C.y = 2.0f + 2.0f * eft_rand_unit();
    work->vel_0x0C.z = 30.0f + 2.0f * eft_rand_unit();
    work->gravity_0x38 = -2.0f;
    rotVecY(&work->vel_0x0C, self->rot_0x24.y);
    if ((s32)self->type_0x02 != 1) {
        work->scale_0x3C = 1.0f;
    } else {
        work->scale_0x3C = 0.3f;
    }
    setVector3(&work->models_0x04[0]->scale_0x1C, work->scale_0x3C, work->scale_0x3C, work->scale_0x3C);
    switch (get_now_mapno()) {
    case 4:
    case 15:
        if ((s32)self->area_0x44 == 0) {
            work->floor_0x48 = -1170.0f;
        } else {
            work->floor_0x48 = 0.0f;
        }
        break;
    default:
        work->floor_0x48 = 0.0f;
        break;
    }
    fn_802F3D94(self);
}

/* Runs the tag-34 orb by its phase: it falls to the floor and lands (0), waits for the charge act (1), hovers and takes
 * the player's hits while the throw motions start their phases (2), wanders around its anchor (3), flies its throw (4)
 * and rides the weapon back (5); then it takes the player's colour (grey while hit) and the ray, and spawns. */
extern "C" void fn_802F3D94(_EFT* self)
{
    _PLW* plw;
    Eft034Work* work;
    _GXColor col;
    s32 hits;
    s32 frames;
    f32 target_scale;
    f32 dist;
    u32 c;
    u32 motion;
    s32 i;
    s32 need;

    plw = (_PLW*)self->source_0x30;
    nw4r::math::VEC3 wander;
    nw4r::math::VEC3 fx;
    work = (Eft034Work*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->timer_0x0C++;
    switch (self->field_0x06) {
    case 0:
        eft034_orb_move(self);
        if (self->pos_0x18.y < work->floor_0x48 - 20.0f) {
            self->pos_0x18.y = work->floor_0x48 - 20.0f;
            work->target_0x18 = self->pos_0x18;
            work->vel_0x0C.x = 0.0f;
            work->vel_0x0C.y = 0.0f;
            work->vel_0x0C.z = 0.0f;
            self->timer_0x0C = 0;
            self->field_0x06++;
            fx = work->target_0x18;
            fx.y = 5.0f + work->floor_0x48;
            EFT_GROUND_BURST(2, &fx, 0.6f, 0, plw->area_0x16, 1);
            EFT_GROUND_BURST(1, &fx, 0.6f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_req_orb(0, &fx);
            }
        }
        break;
    case 1:
        if (pl_status_bit_ck((struct _ENEMY_WORK*)plw, 0x800) == 0) {
            self->state_0x05++;
            return;
        }
        if (Get_motion_no(plw) == 339) {
            self->field_0x06++;
        }
        break;
    case 2:
        if (pl_status_bit_ck((struct _ENEMY_WORK*)plw, 0x800) == 0) {
            self->state_0x05++;
            return;
        }
        if (work->timer_0x30 < plw->field_0x59A || ((_EFT034_PL_CHARGE*)plw)->stage_0x597 > work->stage_0x43) {
            fx = work->target_0x18;
            self->pos_0x18 = fx;
            fx.y = 5.0f + work->floor_0x48;
            EFT_GROUND_BURST(2, &fx, 1.2f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_req_orb(1, &fx);
            }
            hits = plw->field_0x59A;
            if (hits > 20) {
                hits = 20;
            }
            work->vel_0x0C.y = -(20.0f + hits);
            self->demo_flag_0x08 |= 0x80;
        }
        work->timer_0x30 = plw->field_0x59A;
        if (work->timer_0x30 <= 0) {
            self->demo_flag_0x08 &= 0x7F;
        }
        if (self->pos_0x18.y >= work->target_0x18.y) {
            self->pos_0x18.y = work->target_0x18.y;
            work->vel_0x0C.y *= 0.2f;
            work->gravity_0x38 = 2.0f;
        } else {
            work->gravity_0x38 = 1.0f;
        }
        if (((_EFT034_PL_CHARGE*)plw)->stage_0x597 != 0 && eft034_throw_act_ck(self) == 1) {
            if (work->jitter_0x44 < 0 && self->pos_0x18.y >= work->target_0x18.y - 2.0f) {
                if (--self->field_0x10 > 0) {
                    self->pos_0x18.x = work->target_0x18.x - 1.0f + (ran_suu(0) & 3);
                    self->pos_0x18.z = work->target_0x18.z - 1.0f + (ran_suu(0) & 3);
                } else {
                    work->jitter_0x44 = (ran_suu(0) + 20) & 0xF;
                    self->field_0x10 = 10;
                }
            } else {
                work->jitter_0x44--;
            }
        }
        switch (Get_motion_no(plw)) {
        case 340:
            self->demo_flag_0x08 &= 0x7F;
            self->field_0x06++;
            work->vel_0x0C.x = self->pos_0x18.x - plw->vec_0x03C.x;
            work->vel_0x0C.z = self->pos_0x18.z - plw->vec_0x03C.z;
            vec3_normalize_into(&work->vel_0x0C, &work->vel_0x0C);
            switch (plw->armed_motion_0x59C) {
            case 210:
            case 211:
            case 215:
                work->scaled_0x40 = 0;
                break;
            default:
                vec3_scale_in_place(&work->vel_0x0C, 3.0f);
                work->scaled_0x40 = 1;
                break;
            }
            rotVecY(&work->vel_0x0C, (ran_suu(0) & 0x3F) - 32);
            break;
        case 341:
            self->demo_flag_0x08 &= 0x7F;
            eft034_throw_start(self, 0);
            break;
        case 342:
            if (Pl_frame_check(plw, 1, 26.0f, 0.0f) != 0) {
                fx = work->target_0x18;
                fx.y = 5.0f + work->floor_0x48;
                EFT_GROUND_BURST(2, &fx, 0.6f, 0, plw->area_0x16, 1);
                EFT_GROUND_BURST(1, &fx, 0.6f, 0, plw->area_0x16, 1);
                if (self->area_0x44 == get_now_areano()) {
                    se_req_orb(1, &fx);
                }
                self->demo_flag_0x08 &= 0x7F;
                eft034_throw_start(self, 1);
            }
            break;
        case 343:
            self->demo_flag_0x08 &= 0x7F;
            eft034_throw_start(self, 2);
            break;
        }
        eft034_orb_move(self);
        break;
    case 3:
        wander = work->vel_0x0C;
        if ((s32)work->scaled_0x40 == 0) {
            vec3_scale_in_place(&wander, 0.2f);
            rotVecY(&wander, (ran_suu(0) & 0x7FFF) - 0x4000);
        } else {
            vec3_scale_in_place(&wander, 0.5f);
            rotVecY(&wander, (ran_suu(0) & 0x3FFF) - 0x2000);
        }
        work->vel_0x0C += wander;
        if (vec3_len((f32*)&work->vel_0x0C) > 400.0f) {
            vec3_normalize_into(&work->vel_0x0C, &work->vel_0x0C);
            vec3_scale_in_place(&work->vel_0x0C, 400.0f);
        }
        if (self->pos_0x18.y >= work->target_0x18.y) {
            self->pos_0x18.y = work->target_0x18.y;
            work->vel_0x0C.y *= 0.2f;
            work->gravity_0x38 = 2.0f;
        } else {
            work->gravity_0x38 = 1.0f;
        }
        if ((self->timer_0x0C & 7) == 0) {
            fx = self->pos_0x18;
            fx.y = work->target_0x18.y;
            EFT_GROUND_BURST(2, &fx, 1.2f, 0, plw->area_0x16, 1);
            EFT_GROUND_BURST(1, &fx, 1.2f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_slot_req_pos(0, 37, &fx);
            }
        }
        eft034_orb_move(self);
        dist = vec3_distance(&self->pos_0x18, &work->target_0x18);
        if (dist > 100.0f) {
            wander = *(&(self->pos_0x18 - work->target_0x18));
            vec3_scale_in_place(&wander, 100.0f / dist);
            self->pos_0x18 = *(&(work->target_0x18 + wander));
        }
        switch (Get_motion_no(plw)) {
        case 340:
            break;
        case 341:
            if (Pl_frame_check(plw, 1, 16.0f, 0.0f) != 0) {
                work->phase_0x41 = 0;
                fx = self->pos_0x18;
                fx.y = 5.0f + work->floor_0x48;
                eft034_throw_start(self, 0);
                EFT_GROUND_BURST(2, &fx, 2.5f, 0, plw->area_0x16, 1);
                EFT_GROUND_BURST(13, &fx, 1.4f, 0, plw->area_0x16, 1);
                if (self->area_0x44 == get_now_areano()) {
                    se_req_orb(2, &fx);
                }
            }
            break;
        default:
            self->state_0x05++;
            return;
        }
        break;
    case 4:
        motion = Get_motion_no(plw);
        if ((work->phase_0x41 == 0 && motion == 341) || (work->phase_0x41 == 1 && motion == 342) ||
            (work->phase_0x41 == 2 && motion == 343)) {
            work->timer_0x30++;
            switch (work->phase_0x41) {
            case 0:
                frames = 35;
                target_scale = 0.3f;
                break;
            case 1:
                frames = 27;
                target_scale = 0.5f;
                break;
            default:
                frames = 40;
                target_scale = 0.5f;
                break;
            }
            if ((s32)self->type_0x02 != 1) {
                if (work->scale_0x3C > target_scale) {
                    work->scale_0x3C -= (1.0f - target_scale) / frames;
                    if (work->scale_0x3C < target_scale) {
                        work->scale_0x3C = target_scale;
                    }
                }
            } else {
                target_scale = 0.3f;
            }
            if (work->phase_0x41 <= 1) {
                if (work->timer_0x30 >= frames) {
                    self->field_0x06++;
                    ((_EFT034_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
                    self->pos_0x18.y -= 10.0f;
                    work->timer_0x30 = 0;
                    work->vel_0x0C.x = 0.0f;
                    work->vel_0x0C.y = 0.0f;
                    work->vel_0x0C.z = 0.0f;
                    work->gravity_0x38 = 0.0f;
                    work->scale_0x3C = target_scale;
                }
            } else if (work->timer_0x30 < frames) {
                fx.x = work->target_0x18.x - self->pos_0x18.x;
                fx.y = work->target_0x18.y - self->pos_0x18.y;
                fx.z = work->target_0x18.z - self->pos_0x18.z;
                work->vel_0x0C.x = 0.09f * fx.x;
                work->vel_0x0C.y = 0.09f * fx.y;
                work->vel_0x0C.z = 0.09f * fx.z;
            } else {
                self->state_0x05++;
                return;
            }
            eft034_orb_move(self);
            break;
        }
        self->state_0x05++;
        return;
    case 5:
        motion = Get_motion_no(plw);
        if ((work->phase_0x41 == 0 && motion == 341) || (work->phase_0x41 == 1 && motion == 342)) {
            frames = (s32)work->phase_0x41 == 0 ? 50 : 20;
            if (++work->timer_0x30 < frames) {
                ((_EFT034_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
                self->pos_0x18.y -= 10.0f;
            } else {
                self->state_0x05++;
            }
            break;
        }
        self->state_0x05++;
        return;
    }
    work->stage_0x43 = ((_EFT034_PL_CHARGE*)plw)->stage_0x597;
    if (self->demo_flag_0x08 & 0x80) {
        if (eft034_throw_act_ck(self) == 1) {
            need = (Pl_Skill_ck((_PLW*)self->source_0x30, 149) == 1) + 2;
            if (((_EFT034_PL_CHARGE*)plw)->stage_0x597 >= (u8)need && work->vel_0x0C.y > 0.0f) {
                work->vel_0x0C.y = 0.0f;
            }
        } else if (work->vel_0x0C.y > 0.0f) {
            work->vel_0x0C.y = 0.0f;
        }
        col.r = 95;
        col.g = 95;
        col.b = 95;
        col.a = 255;
    } else {
        c = player_color_get(plw->chunk_ofs);
        col.r = c >> 24;
        col.g = (c >> 16) & 0xFF;
        col.b = (c >> 8) & 0xFF;
        col.a = c;
    }
    for (i = 0; i < 2; i++) {
        work->models_0x04[0]->setTevKColor(i, (_GXTevKColorID)3, &col);
    }
    work->models_0x04[0]->pos_0x04 = self->pos_0x18;
    work->models_0x04[0]->field_0x2C = self->rot_0x24.y;
    eft034_ray_flicker(self);
    work->models_0x04[0]->move(0);
    eft_res_models_spawn(self, (void**)&work->models_0x04[0], 2, 1, 0);
}

/* Advances the tag-34 record's state index. */
extern "C" void fn_802F49C8(_EFT* self)
{
    self->state_0x05++;
}

/* Retires the tag-34 record once the effect is over. */
extern "C" void fn_802F49D8(_EFT* self)
{
    eft_res_slot_release(self);
}

/* While the orb hovers (phase 3) it flickers its ray at random for four frames: the ray stretches from the player's
 * weapon tip to the orb. */
extern "C" void eft034_ray_flicker(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;
    nw4r::math::VEC3 tip;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 unused;
    nw4r::math::MTX34 mtx;
    f32 len;
    u32 ang_x;
    u32 ang_y;

    if ((s32)self->field_0x06 == 3) {
        if ((s32)work->ray_on_0x42 == 0) {
            if ((ran_suu(0) & 7) == 0) {
                work->ray_on_0x42 = 1;
                work->ray_timer_0x34 = 0;
            }
        } else if (++work->ray_timer_0x34 >= 4) {
            work->ray_on_0x42 = 0;
        }
    } else {
        work->ray_on_0x42 = 0;
    }
    if (work->ray_on_0x42 == 1) {
        eft034_ray_origin_get(self, &tip);
        dir = *(&(self->pos_0x18 - tip));
        len = vec3_len((f32*)&dir);
        mtx34_identity(&mtx);
        calcVecAngXY(&dir, &ang_x, &ang_y);
        rotMatrixX(ang_x + 0xC000, &mtx);
        rotMatrixY(ang_y, &mtx);
        mtx34_set_trans(&mtx, &tip);
        work->models_0x04[1]->scale_0x1C.y = len;
        work->models_0x04[1]->move2(&mtx, 0);
        eft_res_models_spawn(self, (void**)&work->models_0x04[1], 2, 1, 0);
    }
}

/* Writes the ray's origin: the player's weapon joint 11 moved by the swing's tip offset (one of three, by the attack
 * motion and its frame). */
extern "C" void eft034_ray_origin_get(_EFT* self, nw4r::math::VEC3* out)
{
    _PLW* plw = (_PLW*)self->source_0x30;
    nw4r::math::MTX34 joint;
    nw4r::math::MTX34 rot;
    s32 pose;

    nw4r::math::VEC3 base;
    nw4r::math::VEC3 tip;
    switch (Get_motion_no(plw)) {
    case 0x154:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 0;
        } else if (Pl_frame_check(plw, 2, 30.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 2;
        }
        break;
    case 0x155:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 2, 26.0f, 0.0f) != 0) {
            pose = 2;
        } else if (Pl_frame_check(plw, 2, 28.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 0;
        }
        break;
    case 0x156:
        if (Pl_frame_check(plw, 2, 36.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 2, 40.0f, 0.0f) != 0) {
            pose = 2;
        } else if (Pl_frame_check(plw, 2, 42.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 0;
        }
        break;
    case 0x157:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 1, 22.0f, 0.0f) != 0) {
            pose = 2;
        } else {
            pose = 0;
        }
        break;
    default:
        pose = 0;
        break;
    }
    mhchar_joint_mtx_get(&((_EFT034_PL_PHYS*)plw->physics_0x13C)->chr, 11, (Mtx34*)&joint);
    base.x = -9.2f;
    base.y = -1.7f;
    base.z = 12.0f;
    mulVecMat(&base, &joint);
    mtx34_identity(&rot);
    rotMatrixZ(0xC001, &rot);
    mtx34_concat_assign(&joint, &rot);
    tip.x = eft034_ray_tip_offsets[pose].x;
    tip.y = eft034_ray_tip_offsets[pose].y;
    tip.z = eft034_ray_tip_offsets[pose].z;
    mulVecMat(&tip, &joint);
    out->x = tip.x + (joint.m[0][3] + base.x);
    out->y = tip.y + (joint.m[1][3] + base.y);
    out->z = tip.z + (joint.m[2][3] + base.z);
}

/* Starts the orb's throw `phase`: the target sits in front of the player, the velocity carries it there over 35 (phase
 * 0) or 27 frames (phase 1) against the gravity; phase 2 stops it. */
extern "C" void eft034_throw_start(_EFT* self, u8 phase)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    _PLW* plw;
    Eft034Work* work;
    f32 dist;

    plw = (_PLW*)self->source_0x30;
    work = (Eft034Work*)self->work_0x38;
    work->phase_0x41 = phase;
    ofs.x = 35.0f;
    ofs.y = 140.0f;
    ofs.z = 100.0f;
    rotVecY(&ofs, plw->field_0x058);
    work->target_0x18 = *(&(plw->vec_0x03C + ofs));
    self->field_0x06 = 4;
    from.x = self->pos_0x18.x;
    from.y = 0.0f;
    from.z = self->pos_0x18.z;
    to.x = work->target_0x18.x;
    to.y = 0.0f;
    to.z = work->target_0x18.z;
    switch (work->phase_0x41) {
    case 0:
        work->gravity_0x38 = -2.0f;
        ofs = *(&(to - from));
        dist = vec3_len((f32*)&ofs);
        if (dist > 0.001f) {
            vec3_normalize_into(&ofs, &ofs);
            dist = dist / 35.0f;
            work->vel_0x0C = *(&(ofs * dist));
            work->vel_0x0C.y = (work->target_0x18.y - self->pos_0x18.y) / 35.0f - 35.0f * (0.5f * work->gravity_0x38);
        } else {
            work->vel_0x0C.y = 0.0f;
        }
        break;
    case 1:
        work->gravity_0x38 = -1.0f;
        ofs = *(&(to - from));
        dist = vec3_len((f32*)&ofs);
        if (dist > 0.001f) {
            vec3_normalize_into(&ofs, &ofs);
            dist = dist / 27.0f;
            work->vel_0x0C = *(&(ofs * dist));
            work->vel_0x0C.y = (work->target_0x18.y - self->pos_0x18.y) / 27.0f - 27.0f * (0.5f * work->gravity_0x38);
        } else {
            work->vel_0x0C.y = 0.0f;
        }
        break;
    default:
        setVector3(&work->vel_0x0C, 0.0f, 0.0f, 0.0f);
        work->gravity_0x38 = 0.0f;
        break;
    }
    work->timer_0x30 = 0;
}

/* Moves the orb by its velocity and lets the gravity act on the velocity. */
extern "C" void eft034_orb_move(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;

    self->pos_0x18 += work->vel_0x0C;
    work->vel_0x0C.y += work->gravity_0x38;
}

/* Whether the player's act is one of the orb throws (0x15F, 0x161, 0x163, 0x185, 0x186). */
extern "C" u32 eft034_throw_act_ck(_EFT* self)
{
    switch (((_PLW*)self->source_0x30)->field_0x306) {
    case 0x15F:
    case 0x163:
    case 0x161:
    case 0x185:
    case 0x186:
        return TRUE;
    }
    return FALSE;
}
