#ifndef MHTRI_EF_EFT004_H
#define MHTRI_EF_EFT004_H

#include "types.h"
#include "nw4r/math.h"

/* Declarations for the symbols `src/ef/eft004.cpp` owns (docs/plan.md 6.5, rule 2).  Plain C-linkage
 * names, so C-visible.  Kept minimal.
 *
 * `fn_8010140C` is NOT here: its consumers declare it with different arities (2 args in `eft007.cpp`,
 * 3 in `fn_80114E34.cpp`), so a single declaration cannot serve them - reported as a rule-2 conflict.
 */
#ifdef __cplusplus
extern "C" {
#endif

void fn_80101428(MTX34* out, VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_H */
