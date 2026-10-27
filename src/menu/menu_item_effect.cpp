/*
 * menu/menu_item_effect.cpp - the eft027 `_EFT` effect family (`_EFT::field_0x03 = 27`): four models that circle
 *   the map's spots and dart away from the player and nearby enemies.
 * RANGE. .text 0x8031DAA8-0x8031E57C (6 functions); extab 0x80016034-0x80016054, extabindex 0x80034EB4-0x80034EE4,
 *   .data 0x805DCF38-0x805DD248, .sdata 0x80792C48-0x80792C50, .sdata2 0x8079AE70-0x8079AE98.  The next TU
 *   (`ef/eft044.cpp`) restarts the pool at 0x8079AE98 with its own 1.0f and int-to-float double.
 * FLAGS. `cflags_menu` (configure.py); file-scope `#pragma pool_data off` (retail addresses each spot table with its
 *   own lis/addi in `eft027_place`).
 * NAMES. The file name is a GUESS (no `__FILE__` string covers the range); the bodies are effect code (`_EFT`).
 *   GUESS, from the bodies: `eft027_release`, `eft027_dispatch`, `eft027_place`, `eft027_move`, `eft027_state_step`,
 *   `eft027_retire`; the data `eft027_spots_area12`, `eft027_spots_area2`/`3`/`4`/`6`/`8`, `eft027_joints_area6`,
 *   `eft027_model_ids`.  `menu_area_fx_set` (the eft027 setter) keeps its map name.
 * RESIDUALS.
 *  - `eft027_move`: register colouring - `near`/`i` (r16/r20) swap, the joint byte and the joint-list pointer
 *    (r15/r21) swap, the 90000.0f constant and `radius` (f26/f28) swap; and the `kind` switch's case-2 test is
 *    `cmplwi` where retail has `cmpwi` (the `(u32)` switch is the one spelling that gives retail's `cmplwi` range
 *    test and size).  No declaration order (hill climb over every local), loop-variable sharing or scope reaches it,
 *    nor the enemy-reach product as `radius * r`, `radius *= r` or a separate `reach` local (all unchanged).
 *   flipcheck: `eft027_move`'s bytes (32 of 2772 `.text` bytes); `.data`/`.sdata`/`.sdata2`/extab/extabindex
 *    byte-identical.
 * SHAPES. `eft027_move` declares every local at the top; `radius` doubles as the enemy reach (retail keeps both in
 *   f28).
 */

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8/mtx.h"
#include "mh3_pad.h"
#include "ef.h"
#include "pl.h"
#include "Pl/plw.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/EnemyData.h"
#include "enemy/fn_8012BDF4.h"        /* em_area_ck, em_work_die_ck */
#include "enemy/fn_8012EC74.h"        /* get_em_chg_scale, get_joint_wmat_em */
#include "enemy/fn_801251D0.h"        /* get_enemy_data */
#include "enemy/em_flag_0x8AD_ck.h"
#include "fn_8004CAD8.h"              /* setVector3, rotVecY, calcVecAng2, addVec3, vec3_dist_sq, getKeyData, ... */
#include "ef/eft_res.h"
#include "ef/effect.h"                /* eft_state_flags_set */
#include "ef/fn_800CDB2C.h"           /* ran_suu */
#include "g3d/g3d_calcworld.h"        /* addVec3To */
#include "ef/get_move_work_adrs.h"    /* get_move_work_adrs / get_move_work_max */
#include "stage/get_now_mapno.h"
#include "stage/stg_w.h"              /* get_now_areano */
#include "menu/menu_area_fx_set.h"

#pragma pool_data off

/* One spot an eft027 part circles: the centre and the outer radius (the table closes with a negative radius). */
struct Eft027Spot {
    /* +0x00 */ Vec pos;
    /* +0x0C */ f32 radius;
}; /* size: 0x10 */

/* One eft027 part: its pooled model, the phase it runs and the orbit it flies. */
struct Eft027Part {
    /* +0x00 */ MHchar* model;
    /* +0x04 */ u8 phase;             /* 0 pick a spot, 1 circle it, 2 dart away, 3 wait hidden */
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ VEC3 centre;          /* the spot being circled */
    /* +0x14 */ VEC3 vel;             /* the dart velocity (x only) */
    /* +0x20 */ VEC3 accel;           /* added to `vel` every dart frame */
    /* +0x2C */ u16 turn_speed;       /* the orbit's angle step, kept within 128..150 */
    /* +0x2E */ u16 turn_accel;       /* 0, 1 or 0xFFFF, redrawn whenever `timer` runs out */
    /* +0x30 */ s16 timer;
    /* +0x32 */ u16 heading;          /* the escape heading, away from the player */
    /* +0x34 */ f32 orbit_radius;
}; /* size: 0x38 */

/* eft027's work block, the `_EFT::work_0x38` record `eft_res_slot_get(248)` pools with the effect. */
struct Eft027Work {
    /* +0x00 */ s32 count;
    /* +0x04 */ Eft027Part parts[4];
    /* +0xE4 */ u8 spot_round;        /* the parts that took a spot this round */
    /* +0xE5 */ s8 spot;              /* the spot the next part takes */
    /* +0xE6 */ s8 last_spot;
    /* +0xE7 */ u8 pad_0xE7;
    /* +0xE8 */ Eft027Spot* spots;    /* the area's spot table; NULL retires the effect */
    /* +0xEC */ u8 kind;              /* indexes `eft027_model_ids` and picks the minimum orbit */
    /* +0xED */ u8 pad_0xED[0x3];
    /* +0xF0 */ u8* joints;           /* the shown joints to pick from, closed by 0xFF; NULL: 1..3 */
    /* +0xF4 */ u8 spot_count;
    /* +0xF5 */ u8 pad_0xF5[0x3];
}; /* size: 0xF8 */

extern "C" void eft027_release(_EFT* self);
extern "C" void eft027_dispatch(_EFT* self);
extern "C" void eft027_place(_EFT* self);
extern "C" void eft027_move(_EFT* self);
extern "C" void eft027_state_step(_EFT* self);
extern "C" void eft027_retire(_EFT* self);

/* The spots of map 1 / map 12 area 12. */
Eft027Spot eft027_spots_area12[] = {
    { { -2800.0f, -1200.0f, -7500.0f }, 300.0f },
    { { -7700.0f, -1700.0f, -7400.0f }, 500.0f },
    { { -6500.0f, -1000.0f, -3000.0f }, 500.0f },
    { { -900.0f, -1100.0f, -1200.0f }, 500.0f },
    { { -4200.0f, -800.0f, -4600.0f }, 800.0f },
    { { -1400.0f, -900.0f, -3600.0f }, 800.0f },
    { { 400.0f, -800.0f, -5400.0f }, 800.0f },
    { { 2400.0f, -900.0f, -2500.0f }, 800.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The spots of maps 3 / 14, area 2. */
Eft027Spot eft027_spots_area2[] = {
    { { 1090.0f, -700.0f, 1430.0f }, 700.0f },
    { { -1500.0f, -800.0f, -470.0f }, 700.0f },
    { { -3760.0f, -750.0f, -570.0f }, 700.0f },
    { { -2730.0f, -650.0f, 700.0f }, 700.0f },
    { { -1350.0f, -750.0f, 1400.0f }, 700.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The spots of maps 3 / 14, area 3. */
Eft027Spot eft027_spots_area3[] = {
    { { 3500.0f, -750.0f, 0.0f }, 700.0f },
    { { 2200.0f, -750.0f, 1500.0f }, 700.0f },
    { { 6500.0f, -750.0f, 2000.0f }, 700.0f },
    { { 4500.0f, -750.0f, 1800.0f }, 700.0f },
    { { -7500.0f, -750.0f, 3000.0f }, 700.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The spots of maps 3 / 14, area 4. */
Eft027Spot eft027_spots_area4[] = {
    { { -300.0f, -750.0f, -2100.0f }, 700.0f },
    { { 600.0f, -750.0f, -750.0f }, 700.0f },
    { { 1900.0f, -750.0f, -40.0f }, 700.0f },
    { { -230.0f, -750.0f, 550.0f }, 700.0f },
    { { 1460.0f, -750.0f, 1200.0f }, 700.0f },
    { { 200.0f, -750.0f, 2100.0f }, 700.0f },
    { { -1200.0f, -750.0f, 1700.0f }, 700.0f },
    { { -2400.0f, -750.0f, 620.0f }, 700.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The spots of maps 3 / 14, area 6. */
Eft027Spot eft027_spots_area6[] = {
    { { 940.0f, -750.0f, -1250.0f }, 700.0f },
    { { 370.0f, -750.0f, 460.0f }, 700.0f },
    { { -193.0f, -750.0f, 2180.0f }, 700.0f },
    { { 1370.0f, -750.0f, 2900.0f }, 700.0f },
    { { 3020.0f, -750.0f, 1870.0f }, 700.0f },
    { { 2650.0f, -750.0f, 15.0f }, 700.0f },
    { { 2350.0f, -750.0f, -1860.0f }, 700.0f },
    { { 3720.0f, -750.0f, -1580.0f }, 700.0f },
    { { 3100.0f, -750.0f, 3370.0f }, 700.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The spots of maps 3 / 14, area 8. */
Eft027Spot eft027_spots_area8[] = {
    { { 0.0f, -700.0f, 3500.0f }, 700.0f },
    { { 300.0f, -900.0f, 1500.0f }, 700.0f },
    { { 1200.0f, -1000.0f, -700.0f }, 700.0f },
    { { -600.0f, -700.0f, -2000.0f }, 700.0f },
    { { 600.0f, -800.0f, -1900.0f }, 700.0f },
    { { 600.0f, -500.0f, -2900.0f }, 700.0f },
    { { 500.0f, -500.0f, 500.0f }, 700.0f },
    { { -1400.0f, -1000.0f, -2700.0f }, 700.0f },
    { { 0.0f, 0.0f, 0.0f }, -1.0f },
};

/* The joints an area-6 part may show, closed by 0xFF. */
u8 eft027_joints_area6[] = { 1, 2, 0xFF, 0 };

/* The effect model each `Eft027Work::kind` creates. */
u8 eft027_model_ids[] = { 30, 31, 32, 33 };

/* 0x8031DAA8 (0xD4): pools the eft027 record, installs its two hooks and takes a model for each of its four parts. */
extern "C" void menu_area_fx_set(void)
{
    _EFT* eft = (_EFT*)eft_res_slot_get(248);
    if (eft == NULL)
        return;

    eft->dispatch_0x34 = eft027_dispatch;
    eft->release_0x40 = eft027_release;
    Eft027Work* work = (Eft027Work*)eft->work_0x38;
    work->count = 4;
    for (s32 i = 0; i < work->count; i++) {
        work->parts[i].model = (MHchar*)eft_res_model_get();
        if (work->parts[i].model == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->source_0x30 = NULL;
    eft->field_0x03 = 27;
    eft_state_flags_set(eft, 8, 0);
}

/* 0x8031DB7C (0x64): hands every part's pooled model back (the release hook). */
extern "C" void eft027_release(_EFT* self)
{
    s32 i;
    Eft027Work* work = (Eft027Work*)self->work_0x38;
    for (i = 0; i < work->count; i++)
        eft_res_models_release(&work->parts[i], 1);
}

/* 0x8031DBE0 (0x3C): dispatches `state_0x05` to its state handler (the dispatch hook). */
extern "C" void eft027_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft027_place(self);
        return;
    case 1:
        eft027_move(self);
        return;
    case 2:
        eft027_state_step(self);
        return;
    case 3:
        eft027_retire(self);
        return;
    }
}

/* 0x8031DC1C (0x29C): picks the area's spot table, creates the four models and starts the first frame. */
extern "C" void eft027_place(_EFT* self)
{
    Eft027Work* work = (Eft027Work*)self->work_0x38;
    u8 map = get_now_mapno();

    self->area_0x44 = get_now_areano();
    work->spots = NULL;
    work->joints = NULL;
    switch (map) {
    case 1:
        if (self->area_0x44 == 12) {
            work->spots = eft027_spots_area12;
            work->kind = 0;
        }
        break;
    case 12:
        if (self->area_0x44 == 12) {
            work->spots = eft027_spots_area12;
            work->kind = 1;
        }
        break;
    case 3:
    case 14:
        switch (self->area_0x44) {
        case 2:
            work->spots = eft027_spots_area2;
            break;
        case 3:
            work->spots = eft027_spots_area3;
            break;
        case 4:
            work->spots = eft027_spots_area4;
            break;
        case 6:
            work->spots = eft027_spots_area6;
            work->joints = eft027_joints_area6;
            break;
        case 8:
            work->spots = eft027_spots_area8;
            break;
        }
        if (map == 3)
            work->kind = 2;
        else
            work->kind = 3;
        break;
    }
    if (work->spots == NULL) {
        eft027_retire(self);
        return;
    }
    for (s32 i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->parts[i].model, eft027_model_ids[work->kind], 8) == NULL) {
            eft027_retire(self);
            return;
        }
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    work->spot_count = 0;
    for (s32 i = 0; work->spots[i].radius >= 0.0f; i++)
        work->spot_count++;
    for (s32 i = 0; i < work->count; i++) {
        setVector3(&work->parts[i].model->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->parts[i].model->field_0x28 = 0;
        work->parts[i].model->field_0x2C = 0;
        work->parts[i].model->field_0x28 = 0;
        work->parts[i].model->ready = 0;
        work->parts[i].phase = 0;
    }
    work->spot_round = 0;
    work->spot = -1;
    work->last_spot = -1;
    eft027_move(self);
}

/* 0x8031DEB8 (0x6B0): steps every part: pick a spot and an orbit, circle it, dart away from a near player or
 * enemy, then wait hidden. */
extern "C" void eft027_move(_EFT* self)
{
    _PLW* player;
    s32 near;
    u16 enemy_max;
    Eft027Work* work;
    Eft027Part* part;
    _ENEMY_WORK* enemy;
    s32 i;
    u8* joints;
    u8* p;
    u8 joint;
    s32 j;
    s32 k;
    f32 radius;
    f32 min_radius;
    VEC3 offset;
    VEC3 unused;
    VEC3 centre;
    VEC3 pos;

    work = (Eft027Work*)self->work_0x38;
    player = (_PLW*)get_move_work_adrs(2);
    enemy_max = get_move_work_max(3);
    near = 0;
    VEC3_ctor(&offset);
    VEC3_ctor(&unused);
    VEC3_ctor(&centre);
    for (i = 0; i < work->count; i++) {
        part = &work->parts[i];
        enemy = (_ENEMY_WORK*)get_move_work_adrs(3);

        switch (part->phase) {
        case 0:
            if (work->spot_round == 0) {
                while (work->last_spot == work->spot)
                    work->spot = (s8)(ran_suu(1) % work->spot_count);
                work->last_spot = work->spot;
            }
            radius = work->spots[work->spot].radius;
            vec_to_mh_vec3(&centre, &work->spots[work->spot].pos);
            copyVec3(&part->centre, &centre);
            work->spot_round++;
            if (work->spot_round >= 1)
                work->spot_round = 0;
            switch ((u32)work->kind) {
            case 0:
            case 1:
                min_radius = 200.0f;
                break;
            case 2:
                min_radius = 600.0f;
                break;
            }
            part->orbit_radius = min_radius + (ran_suu(1) % (u16)(radius - min_radius));
            part->turn_speed = 64;
            part->turn_accel = 0;
            setVector3(&part->vel, 0.0f, 0.0f, 0.0f);
            setVector3(&part->accel, 0.0f, 0.0f, 0.0f);
            part->timer = ran_suu(1) % 30;
            part->model->field_0x2C += 0x8000;

            joint = 0;
            joints = work->joints;
            if (joints == NULL) {
                joint = ran_suu(1) % 3 + 1;
            } else {
                for (p = joints; *p != 0xFF; p++)
                    joint++;
                joint = joints[ran_suu(1) % joint];
            }
            for (j = 1; j < 4; j++) {
                if (j == joint)
                    part->model->setVisibility(j, true);
                else
                    part->model->setVisibility(j, false);
            }
            part->phase++;
        case 1:
            if (part->timer < 0) {
                switch ((u8)(ran_suu(1) % 10)) {
                case 0:
                    part->turn_accel = 1;
                    break;
                case 1:
                    part->turn_accel = 0xFFFF;
                    break;
                }
                part->timer = ran_suu(1) % 30;
            }
            part->turn_speed += part->turn_accel;
            if (part->turn_speed > 150)
                part->turn_speed = 150;
            else if (part->turn_speed < 128)
                part->turn_speed = 128;
            setVector3(&offset, 0.0f, 0.0f, part->orbit_radius);
            part->model->field_0x2C = (u16)(part->model->field_0x2C + part->turn_speed);
            rotVecY(&offset, part->model->field_0x2C);
            addVec3(&pos, &part->centre, &offset);
            copyVec3(&part->model->pos_0x04, &pos);
            if (vec3_dist_sq(&player->vec_0x03C, &part->model->pos_0x04) < 90000.0f)
                near = 1;
            for (k = 0; k < enemy_max; k++, enemy++) {
                if (enemy->active != 0 && em_area_ck(enemy) && (enemy->field_0x1C8 & 1)) {
                    radius = get_enemy_data(enemy)->size_0x54->radius_0x24;
                    radius = get_enemy_data(enemy)->size_0x54->radius_0x24 * radius;
                    if (vec3_dist_sq(&enemy->pos, &part->model->pos_0x04) < radius)
                        near = 1;
                }
            }
            if (near) {
                part->heading = calcVecAng2(&player->vec_0x03C, &part->model->pos_0x04);
                part->vel.x = 4.0f;
                part->accel.x = 1.5f;
                part->timer = 40;
                part->phase++;
            }
            break;
        case 2:
            part->model->field_0x2C += (u16)((s16)(part->heading - part->model->field_0x2C) >> 4);
            addVec3To(&part->vel, &part->accel);
            if (part->vel.x > 40.0f)
                part->vel.x = 40.0f;
            copyVec3(&offset, &part->vel);
            rotVecY(&offset, part->model->field_0x2C);
            addVec3To(&part->model->pos_0x04, &offset);
            if (part->timer < 0) {
                for (s32 j = 1; j < 4; j++)
                    part->model->setVisibility(j, false);
                part->phase++;
                part->timer = 180;
            }
            break;
        case 3:
            if (part->timer < 0)
                part->phase = 0;
            break;
        }
        part->timer--;
        part->model->move(0);
        eft_res_models_spawn(self, (void**)part, 2, 1, 0);
    }
}

/* 0x8031E568 (0x10): advances the effect to its next state. */
extern "C" void eft027_state_step(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8031E578 (0x4): retires the effect's record. */
extern "C" void eft027_retire(_EFT* self)
{
    eft_res_slot_release(self);
}
