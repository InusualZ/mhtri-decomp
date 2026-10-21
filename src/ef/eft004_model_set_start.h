#ifndef MHTRI_EF_EFT004_MODEL_SET_START_H
#define MHTRI_EF_EFT004_MODEL_SET_START_H

#include "types.h"

/* Leaf header for `eft004_model_set_start`, which `src/ef/em_effect_ctrl.cpp` defines (docs/plan.md 6.5, rule 2; playbook 51).
 * Kept minimal - only the declarations a consumer needs - and C-visible, because these are plain
 * C-linkage symbols.  Where the owner and a consumer view the object through
 * different local types, the parameter is `void*` so one header serves them all without a guess.
 */
#ifdef __cplusplus
extern "C" {
#endif

void eft004_model_set_start(void* self); /* untyped: opaque handle - the owner and its consumers use different local state types */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_MODEL_SET_START_H */
