/*
 * The `fn_80059550.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `src/fn_80059550.cpp` owns the checked global getter `fn_8005A9BC`.  `g3d/g3d_resfile.cpp`'s accessor
 * cluster calls it, so it is declared once here (the owner's header) and that consumer includes it.
 *
 * It keeps C linkage (its map name is a plain `fn_XXXXXXXX` stem).
 */
#ifndef MHTRI_FN_80059550_H
#define MHTRI_FN_80059550_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 fn_8005A9BC(void* p); /* 0x8005A9BC - the checked global getter (asserts the handle, returns its word) */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80059550_H */
