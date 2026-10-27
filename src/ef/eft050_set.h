/* ef/eft050_set.h - `ef/eft050_set.cpp`'s header: the eft050_set record, its work block and its state bodies. */
#ifndef MHTRI_EF_EFT050_SET_H
#define MHTRI_EF_EFT050_SET_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"

/* The block `eft050_set` pools for the family `eft050_set_dispatch` drives: a word count at +0x00, the four
 * pooled handles at +0x04, the extra record the fourth `eft_res_model_get` hands back at +0x14 and the
 * eight-byte run the spawner zeroes at +0x1C. size: 0x2C (the slot size `eft050_set` asks `eft_res_slot_get` for) */
typedef struct Eft050SetWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[4];
    /* +0x14 */ void* extra_0x14;   /* the record the model create fills; NULL retires the effect */
    /* +0x18 */ void* created_0x18; /* what `res_eft_UV_model_create` returned */
    /* +0x1C */ struct _g3d_work* works_0x1C[2]; /* the create's handle list, pushed back in reverse */
    /* +0x24 */ u8 unused_0x24[0x04];
    /* +0x28 */ f32 scale_0x28;     /* the UV model's scale */
} Eft050SetWork;

/* Pools the eft050_set record at `pos` for `plw`, in area `flag`. */
void eft050_set(_PLW* plw, nw4r::math::VEC3* pos, u8 flag);

#ifdef __cplusplus
extern "C" {
#endif

void eft050_set_release(_EFT* self);
void eft050_set_dispatch(_EFT* self);
void eft050_set_create(_EFT* self);
void eft050_set_move(_EFT* self);
void eft_state_advance(_EFT* self);
void eft_instance_release(_EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT050_SET_H */
