/* ef/ef_res_emitterparam_vec.h - leaf header for the resource-slot and emitter-parameter accessors
 * `src/ef/ef_postfield.cpp` defines with C linkage over its one-word handles (docs/plan.md 6.5 rule 2).  A leaf
 * because `ef/ef_postfield.h` is read by C and declares none of them; the handle is opaque here. */
#ifndef MHTRI_EF_EF_RES_EMITTERPARAM_VEC_H
#define MHTRI_EF_EF_RES_EMITTERPARAM_VEC_H

#include "types.h"
#include "nw4r/math.h"

struct EftResHandle; /* a one-word resource handle: the address of the resource block */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800B2544 - stores the emitter description address into the slot. */
void ef_res_slot_set_emitter_desc(struct EftResHandle* self, u8* value);
/* 0x800B2590 - stores the parameter block address into the slot. */
void ef_res_slot_set_params(struct EftResHandle* self, u8* value);
/* 0x800B2504 - binds the slot to the emitter resource's description. */
void ef_res_slot_bind_emitter_desc(struct EftResHandle* self, struct EftResHandle* emitter);
/* 0x800B254C - binds the slot to the emitter resource's parameter block. */
void ef_res_slot_bind_params(struct EftResHandle* self, struct EftResHandle* emitter);
/* 0x800B26B0 - the emitter parameter block's scale. */
f32 ef_res_emitterparam_scale(struct EftResHandle* self);
/* 0x800B26D4 - the emitter parameter block's vector. */
nw4r::math::VEC3* ef_res_emitterparam_vec(struct EftResHandle* self);
/* 0x800B2810 - sets the emitter parameter block's scale. */
void ef_res_emitterparam_set_scale(struct EftResHandle* self, f32 value);
/* 0x800B2840 - copies `src` into the emitter parameter block's vector. */
void ef_res_emitterparam_set_vec(struct EftResHandle* self, const nw4r::math::VEC3* src);
/* 0x800B24BC - sets or clears the draw parameter block's 0x0400 flag. */
void ef_res_drawparam_set_flag_0400(struct EftResHandle* self, int enable);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_RES_EMITTERPARAM_VEC_H */
