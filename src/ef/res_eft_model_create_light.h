/* ef/res_eft_model_create_light.h - leaf header: `res_eft_model_create_light` (0x800F8E68), owned by `ef/eft_res.cpp`.
 * C++ linkage: the map carries `res_eft_model_create_light__FP6MHcharUsUll`. */
#ifndef MHTRI_EF_RES_EFT_MODEL_CREATE_LIGHT_H
#define MHTRI_EF_RES_EFT_MODEL_CREATE_LIGHT_H

#include "types.h"

struct MHchar;

/* Binds the light resource `id` to `model` at joint `light`; returns 0 when the resource is missing. */
u32 res_eft_model_create_light(MHchar* model, u16 id, u32 arg, long light);

#endif
