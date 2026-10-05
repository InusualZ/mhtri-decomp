#ifndef MHTRI_EF_FN_80114E34_H
#define MHTRI_EF_FN_80114E34_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_80114E34.cpp` owns (docs/plan.md 6.5, rule 2; docs/matching.md 51).
 * Kept minimal - only the declarations a consumer needs - and C-visible, because these are plain
 * C-linkage symbols (`fn_XXXXXXXX`).  Where the owner and a consumer view the object through
 * different local types, the parameter is `void*` so one header serves them all without a guess.
 */
#ifdef __cplusplus
extern "C" {
#endif

void fn_80116FCC(void* self);
void fn_80117074(void* self);

/* The eft020 spawner `ef/eft019.cpp`'s actor wrappers call: creates the family-20 record
 * (`_EFT` from `ef.h`) for `(type, area)` and returns it, or 0 when the area check fails. */
struct _EFT;
struct _EFT* fn_80114E34(u8 type, u8 area, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_80114E34_H */
