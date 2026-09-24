#ifndef MHTRI_EF_FN_800AEE48_H
#define MHTRI_EF_FN_800AEE48_H

#include "types.h"
#include "nw4r/math.h"

/* Declarations for the symbols `src/ef/fn_800AEE48.cpp` owns (docs/plan.md 6.5, rule 2).  C-visible;
 * kept minimal - only the declarations a consumer needs.
 *
 * The signatures are the owner's own definitions - `f32 fn_800B5A48(void)`,
 * `int fn_800B59E4(void* self)`, `void* fn_800B4B04(void* self, s16 flag)` - so including this header
 * from the owner cannot conflict.  `fn_800B7DB0` is not reconstructed in the owner unit yet, so it
 * keeps the `void*`/`MTX34*` view the consumers (and `unsplit/ef.h`) already share.
 */
#ifdef __cplusplus
extern "C" {
#endif

f32 fn_800B5A48(void);
int fn_800B59E4(void* self);
void* fn_800B4B04(void* self, s16 flag);
void fn_800B7DB0(void* em, MTX34* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_800AEE48_H */
