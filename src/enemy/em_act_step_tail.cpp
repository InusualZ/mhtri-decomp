/* enemy/em_act_step_tail.cpp - the frame effect's creators: two entry points that pool a one-model effect record
 *   (`_EFT`) for a player or an NPC source, the record's model release and its four-state dispatcher.
 * RANGE. .text 0x80330194-0x8033041C (4 functions); extab, extabindex.  `enemy/em_act_step.cpp` ends with its
 *   static initializer just before 0x80330194; the dispatcher's four state bodies (0x8033041C, 0x803305F8,
 *   0x80330E00, 0x80330E10) open `hud/pl_frame_sync.cpp`, so the TU most likely runs on past 0x8033041C.
 * NAMES. Every symbol is a GUESS from its body and callers (`Pl/pl_act_step.cpp` calls the player creator with
 *   `pl_frame_window_1`, `ai/ai_npc.cpp` the NPC one): eft_frame_pl_create, eft_frame_npc_create,
 *   eft_frame_models_release, eft_frame_state_dispatch; the state bodies eft_frame_state_init/_step/_end/_nop and
 *   `EftFrameModel`/`EftFrameWork`.
 * RESIDUALS. none.
 * SHAPES. File-wide `#pragma peephole off`; the model slot is `volatile` (retail re-reads it after the store);
 *   the loop counter is declared first (register colouring).
 */

#include "types.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "hud/eft_frame_state_init.h" /* eft_frame_state_init / _step / _end / _nop */

/* Retail builds each callback address in r0 (`addi r0,r4,fn@l` + `stw r0`); the peephole pass folds it into r4. */
#pragma peephole off

/* One model slot of the frame effect's work block.  size: 0x34 */
struct EftFrameModel {
    /* +0x00 */ u8* volatile model;              /* the pooled model `eft_res_model_get` hands out */
    /* +0x04 */ u8 unused_0x04[0x2C - 0x04];
    /* +0x2C */ f32 scale;                       /* the creator's scale (first slot only) */
    /* +0x30 */ u8 unused_0x30[0x34 - 0x30];
};

/* The frame effect's work block (`_EFT::work_0x38`): the model count and the slots.  size: 0x38 */
struct EftFrameWork {
    /* +0x00 */ u8 count;
    /* +0x01 */ u8 pad_0x01[0x04 - 0x01];
    /* +0x04 */ EftFrameModel models[1];
};

extern "C" void eft_frame_models_release(_EFT* self);
extern "C" void eft_frame_state_dispatch(_EFT* self);


/* 0x80330194 (0xF0): pools a one-model frame effect for a player source, kind 47. */
extern "C" void eft_frame_pl_create(struct _PLW* source, f32 scale)
{
    s32 i;
    _EFT* eft = (_EFT*)eft_res_slot_get(56);
    EftFrameWork* work;

    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = 0;
    eft->release_0x40 = eft_frame_models_release;
    eft->dispatch_0x34 = eft_frame_state_dispatch;
    work = (EftFrameWork*)eft->work_0x38;
    work->count = 1;
    for (i = 0; i < work->count; i++) {
        work->models[i].model = eft_res_model_get();
        if (work->models[i].model == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    work->models[0].scale = scale;
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft_state_flags_set(eft, 1, 0);
    eft->field_0x03 = 47;
    eft->source_0x30 = source;
    eft->field_0x07 = 0;
}

/* 0x80330284 (0xF0): pools a one-model frame effect for an NPC source, mode 2. */
extern "C" void eft_frame_npc_create(struct _AINPC_W* source, f32 scale)
{
    s32 i;
    _EFT* eft = (_EFT*)eft_res_slot_get(56);
    EftFrameWork* work;

    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = 0;
    eft->release_0x40 = eft_frame_models_release;
    eft->dispatch_0x34 = eft_frame_state_dispatch;
    work = (EftFrameWork*)eft->work_0x38;
    work->count = 1;
    for (i = 0; i < work->count; i++) {
        work->models[i].model = eft_res_model_get();
        if (work->models[i].model == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    work->models[0].scale = scale;
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft_state_flags_set(eft, 1, 0);
    eft->field_0x03 = 0;
    eft->source_0x30 = source;
    eft->field_0x07 = 2;
}

/* 0x80330374 (0x6C): returns every model slot to the pool and clears the count. */
extern "C" void eft_frame_models_release(_EFT* self)
{
    s32 i;
    EftFrameWork* work = (EftFrameWork*)self->work_0x38;

    for (i = 0; i < work->count; i++) {
        eft_res_models_release(&work->models[i], 1);
    }
    work->count = 0;
}

/* 0x803303E0 (0x3C): runs the state body the record's state byte names. */
extern "C" void eft_frame_state_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft_frame_state_init(self);
        break;
    case 1:
        eft_frame_state_step(self);
        break;
    case 2:
        eft_frame_state_end(self);
        break;
    case 3:
        eft_frame_state_nop(self);
        break;
    }
}
