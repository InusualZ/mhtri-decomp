/* Leaf header (docs/plan.md 6.5 rule 2): `res_eft_UV_model_create_name`, which `ef/eft_res.cpp` owns, in the
 * owner's own signature.  A C++ free function (`res_eft_UV_model_create_name__FP6MHcharPcUllPP9_g3d_worklUc`). */
#ifndef MHTRI_EF_RES_EFT_UV_MODEL_CREATE_NAME_H
#define MHTRI_EF_RES_EFT_UV_MODEL_CREATE_NAME_H

#include "types.h"

#ifdef __cplusplus
struct MHchar;
struct _g3d_work;

/* 0x800F9008 - builds the UV-animated model of resource file `name` for `model`, filing its g3d works in `list`. */
/* untyped: opaque handle - the UV animation record the caller stores */
void* res_eft_UV_model_create_name(MHchar* model, char* name, u32 arg, long light, struct _g3d_work** list, long count,
                                   u8 flag);
#endif

#endif /* MHTRI_EF_RES_EFT_UV_MODEL_CREATE_NAME_H */
