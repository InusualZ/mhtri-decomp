#ifndef MHTRI_EF_EM_HIT_EFT_H
#define MHTRI_EF_EM_HIT_EFT_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_80104BD0.c` owns (docs/plan.md 6.5, rule 2; playbook 51).
 * Kept minimal - only the declarations a consumer needs - and C-visible, because these are plain
 * C-linkage symbols (`fn_XXXXXXXX`).  Where the owner and a consumer view the object through
 * different local types, the parameter is `void*` so one header serves them all without a guess.
 */
#ifdef __cplusplus
extern "C" {
#endif

struct _EFT;
void em_hit_eft_step(struct _EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EM_HIT_EFT_H */
