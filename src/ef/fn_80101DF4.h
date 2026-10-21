#ifndef MHTRI_EF_FN_80101DF4_H
#define MHTRI_EF_FN_80101DF4_H

#include "types.h"

/* Leaf header for `fn_80101DF4`, which `src/ef/em_effect_ctrl.cpp` defines (docs/plan.md 6.5, rule 2; playbook 51).
 * Kept minimal - only the declarations a consumer needs - and C-visible, because these are plain
 * C-linkage symbols (`fn_XXXXXXXX`).  Where the owner and a consumer view the object through
 * different local types, the parameter is `void*` so one header serves them all without a guess.
 */
#ifdef __cplusplus
extern "C" {
#endif

void fn_80101DF4(void* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_80101DF4_H */
