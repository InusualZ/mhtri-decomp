/* ef/eft041.cpp - the effect-41 (area effect) machine: the state-0 handler that creates each model from the stage's
 *   ".brres" resource name, the state-1 move step, the state-2 step and the state-3 kill that retires the effect.
 * RANGE. .text 0x803066F0-0x80306A98 (4 functions); extab 0x80015AAC-0x80015ABC, extabindex 0x80034668-0x80034680,
 *   .data 0x805DC5E8-0x805DC618 (`eft041_place_pos_tbl`), .sdata 0x80792B70-0x80792B78 (the ".brres" string),
 *   .sdata2 0x8079ADE0-0x8079ADF0 (1.0f for `eft041_model_create`, 0.0f/20.0f/4.0f for `eft041_move`).  Right edge:
 *   `ef/fn_8030681C.cpp` (the effect-42 family) starts at 0x80306A98 with its first spawner; its `.sdata2` pool pools
 *   0.0f and 1.0f again, which is why the two are separate TUs.  The creator `lb_area_fx_set` and the release and
 *   dispatch hooks (0x80306524-0x803066F0) sit in `ef/eft_em_tint.cpp`'s range and belong to this TU too (seam request
 *   filed).
 * FLAGS. `cflags_main`.  `eft041_model_create` compiles at the library defaults; the pragmas below the first function
 *   (`peephole off`, `optimization_level 4`, `fp_contract off`, `pool_data off`) are what `eft041_move` needs: retail
 *   keeps the unfused `clrlwi` narrowing and compares and `fmuls` + `fadds`.
 * NAMES. `eft041_move`/`eft041_end`/`eft041_kill` are the registry's; `eft041_model_create` and `eft041_place_pos_tbl`
 *   are GUESSes in the same scheme (state 0 `model_create`, 1 `move`, 2 `end`, 3 `kill`).
 * RESIDUALS. `eft041_move` (0x8030681C) is partial: retail keeps the constant 200 in a register for the `++timer > 200`
 *   compare (`cmpw r0,r23`, hoisted out of the loop) where ours compares the immediate, and the callee-saved registers
 *   are numbered differently (a local `limit` and the reversed compare measure the same).
 * SHAPES. `created` in the model record is `void* volatile` for `eft041_model_create`: retail stores the call result
 *   and re-reads it (`stw r3,8(r28); lwz r0,8(r28)`), which no flag set reproduces.  The declarations run
 *   `data; i; name;` (with `name` first the `name`/`i` registers swap).
 * SHAPES. Local declaration order, reordered to the registers regsim's plan gives: eft041_move: scope 0 order
 *   `mapno`, `i`, `pos_num`, `work`, `pos_tbl`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "MSL_C/strcat.h"
#include "ef.h"
#include "ef/eft041_kill.h"
#include "ef/eft041_move.h"
#include "ef/eft041_model_create.h"
#include "ef/res_eft_UV_model_create_name.h"
#include "stage/stage_char_res_body_get.h"

/* The touched tail of the model object `res_eft_UV_model_create_name` returns.  `scale` is set to 1,1,1 by
 * `eft041_model_create`; the three accumulators are the ones `fn_8030627C` adds a masked `ran_suu()` delta to every
 * frame; `ready` is set to 1 by the model creator and again here. */
typedef struct UvModelTail {
    u8   pad_0x00[0x1C];  /* +0x00 */
    f32  scale[3];        /* +0x1C */
    u32  rand_accum_a;    /* +0x28 */
    u32  rand_accum_b;    /* +0x2C */
    u32  rand_accum_c;    /* +0x30 */
    u8   pad_0x34;        /* +0x34 */
    u8   ready;           /* +0x35 */
} UvModelTail;                 /* size: 0x36 (the model object continues past +0x36) */

/* One model record of the work area as `eft041_model_create` writes it: the resource handle and the created model. */
typedef struct EftCreateModel {
    UvModelTail* model;          /* +0x00 */
    void* volatile created;      /* +0x04 */
} EftCreateModel;                /* size: 0x08 */

/* The work area at `_EFT::work_0x38` as the creator fills it: the model count, the model records and their four
 * `_g3d_work` slots, plus the area resource.  size: 0x20 */
typedef struct EftCreateWork {
    u32            count;       /* +0x00 */
    EftCreateModel models[1];   /* +0x04 */
    struct _g3d_work* works[1][4]; /* +0x0C */
    void*          resource;    /* +0x1C */
} EftCreateWork;                /* size: 0x20 */

/* Creates the effect's models from the stage's ".brres" resource name, resets the model transforms and the step
 * timers, then advances the effect to its next state and runs that state's handler. */
extern "C" void eft041_model_create(_EFT* self)
{
    EftCreateWork* data;
    u32 i;
    char* name;

    data = (EftCreateWork*)self->work_0x38;
    name = (char*)stage_char_res_body_get(0);
    strcat(name, ".brres");

    for (i = 0; i < data->count; i++) {
        data->models[i].created = res_eft_UV_model_create_name(
            (MHchar*)data->models[i].model, name, 0x108, 0, &data->works[i][0], 3, 2);
        if (data->models[i].created == NULL) {
            eft041_kill(self);
            return;
        }
    }

    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;

    for (i = 0; i < data->count; i++) {
        setVector3((nw4r::math::VEC3*)data->models[i].model->scale, 1.0f, 1.0f, 1.0f);
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->rand_accum_b = 0;
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->ready = 1;
    }

    eft041_move(self);
}

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off
#pragma pool_data off
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */

#include "nw4r/math.h"
#include "gx.h"

#include "ef.h"
#include "unsplit/ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "sound/fn_800DD1F0.h"
#include "menu/menu_message.h"
#include "pl.h"
#include "fn_8004CAD8.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "enemy/fn_8012BDF4.h"
#include "unsplit/enemy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "menu/menu_infomation.h"
#include "ef/eft004_set.h"
#include "rotLocalMatX.h"
#include "sound/fn_800D7F54.h"
#include "stage/get_stg_eft_col.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/g3d/scnmdl.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcmaterial.h"
#include "sound/copied_mat_access_valid_ck.h"
#include "ef/eft_stage_spawn_flag.h"
#include "Pl/Pl_master_ck.h"
#include "ef/eft042_work.h"

/* the stage-2/13 area-10 placements `eft041_move` picks from */
Vec eft041_place_pos_tbl[4] = {
    {98.0f, -105.0f, -500.0f}, {-950.0f, -30.0f, 1400.0f},
    {-3040.0f, -65.0f, -85.0f}, {-4900.0f, 0.0f, -1000.0f},
};

/* ---------------------------------------------------------------------------------------------------
 * 0x8030681C - the effect-41 machine's state-1 step
 * ------------------------------------------------------------------------------------------------- */

/* Walks the work area's model records: places each model when its step timer expires, reveals its joints, walks it
 * and retires it after the second timer, then hands its position to `eft004_set`. */
extern "C" void eft041_move(_EFT* self)
{
    u8 mapno;
    u32 i;
    u32 pos_num;
    Eft042Work* work;
    Vec* pos_tbl;
    work = (Eft042Work*)self->work_0x38;
    mapno = get_now_mapno();
    nw4r::math::VEC3 pos;

    pos_tbl = NULL;
    if ((mapno == 2 || mapno == 13) && self->area_0x44 == 10) {
        pos_tbl = eft041_place_pos_tbl;
        pos_num = 4;
    }

    for (i = 0; i < work->count; i++) {
        switch (self->field_0x06) {
        case 0:
            if (--self->timer_0x0C < 0) {
                u8 idx;
                u32 joint;
                u32 joints;

                self->field_0x06++;
                ((EftUvModelObject*)work->models[i].v_0x04.created)->set_frame(0.0f);

                idx = ran_suu(1) % pos_num;
                vec_to_mh_vec3(&work->models[i].model->pos_0x04, &pos_tbl[idx]);

                joints = work->models[i].model->get_joint_num();
                for (joint = 0; joint < joints; joint++) {
                    if (joint == ((u8*)work->resource)[idx]) {
                        work->models[i].model->setVisibility(joint, true);
                    } else {
                        work->models[i].model->setVisibility(joint, false);
                    }
                }
            }
            break;
        case 1:
            if (++self->timer_0x0C > 200) {
                self->field_0x06 = 0;
                self->timer_0x0C = ran_suu(1) & 0x1F;
            } else {
                work->models[i].model->move(0);
                ((EftUvModelObject*)work->models[i].v_0x04.created)->update();
                eft_res_models_spawn(self, (void**)&work->models[i], 2, 1, NULL);
            }
            break;
        }

        switch (200 - self->timer_0x0C) {
        case 15:
        case 20:
        case 25:
        case 30:
        case 35:
            se_bank46_id13_req(&work->models[i].model->pos_0x04);
            break;
        }

        pos = work->models[i].model->pos_0x04;
        pos.y -= 20.0f;
        eft004_set(0, &pos, 4.0f, 0, self->area_0x44);
    }
}

/* The machine's state-2 step: one step further into the effect. */
extern "C" void eft041_end(_EFT* self)
{
    self->state_0x05++;
}

/* The machine's state-3 step (and every failure path): retire the effect. */
extern "C" void eft041_kill(_EFT* self)
{
    eft_res_slot_release(self);
}

