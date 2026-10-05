/* ef/eft_res_spawn_gate_ck.h - the declaration of `eft_res_spawn_gate_ck`, which `ef/eft_res.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT_RES_SPAWN_GATE_CK_H
#define MHTRI_EF_EFT_RES_SPAWN_GATE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* untyped: an opaque handle passed through (the callee casts it to the effect record) */
u32 eft_res_spawn_gate_ck(void* self_, u32 mode);
#ifdef __cplusplus
}
#endif

#endif
