/* ef/eft_model_slot.cpp - the effect model-slot handlers
 *
 * `.text` 0x800FACAC..0x800FAE08, 5 functions written (the rest of the range is not decompiled yet).
 * Name is a GUESS: the five functions release or update an effect's model slot (`EftModelOwner`, handler at slot +0x34).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/scnmdl.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "g3d/fn_80063888.h" /* fn_80064820, owned by g3d/fn_80063888.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "ef/effect_types.h"

/* One pool slot `fn_800FBD68` returns. */
typedef struct EftPoolSlot {
    /* +0x00 */ u8 pad_0x00[0x03];
    /* +0x03 */ u8 state_0x03;
    /* +0x04 */ u8 pad_0x04[0x2C];
    /* +0x30 */ void* owner_0x30;
    /* +0x34 */ void* handler_0x34;
} EftPoolSlot; /* size: 0x38 */

/* The model record `fn_800FAD90` releases. */
typedef struct EftJointModel {
    /* +0x00 */ void* field_0x00;
    /* +0x04 */ void* field_0x04;
} EftJointModel; /* size: 0x08 */

/* The owner whose model pointer sits at +0x38. */
typedef struct EftModelOwner {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ EftJointModel* model_0x38;
} EftModelOwner; /* size: 0x3C */

extern "C" {
void fn_800F8A44(void* a, void* b);

void* fn_800FBD68(s32 a);

/* the next proposal's dispatcher entries `fn_800FADCC` tail-calls */
void fn_800FAE08(void* self);
void fn_800FB160(void* self);
void fn_800FBBAC(void* self);
void fn_800FBBBC(void* self);

void fn_800FADCC(EftFrameState* self);
}

#pragma fp_contract off
#pragma peephole off

/* 0x800FACAC - register the area owner on pool slot 0. */
extern "C" void fn_800FACAC(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(0);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
    }
}

/* 0x800FACF0 - register the area owner and the slot-1 handler. */
extern "C" void fn_800FACF0(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(1);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
        slot->handler_0x34 = (void*)fn_800FADCC;
    }
}

/* 0x800FAD40 - register the area owner and the slot-2 handler. */
extern "C" void fn_800FAD40(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(2);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
        slot->handler_0x34 = (void*)fn_800FADCC;
    }
}

/* 0x800FAD90 - release the model held by the owner. */
extern "C" void fn_800FAD90(EftModelOwner* self) {
    EftJointModel* model = self->model_0x38;
    fn_800F8A44(&model->field_0x04, model->field_0x00);
    model->field_0x00 = NULL;
}

/* 0x800FADCC - dispatch on the effect type byte. */
extern "C" void fn_800FADCC(EftFrameState* self) {
    switch (self->mode_0x05) {
    case 0:
        fn_800FAE08(self);
        break;
    case 1:
        fn_800FB160(self);
        break;
    case 2:
        fn_800FBBAC(self);
        break;
    case 3:
        fn_800FBBBC(self);
        break;
    default:
        break;
    }
}
