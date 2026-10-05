/* The player act state-machine band `Pl/fn_80258FCC.cpp` (`.text` 0x80258FCC-0x8025F088).
 *
 * Declarations for the symbols this unit owns that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_PL_FN_80258FCC_H
#define MHTRI_PL_FN_80258FCC_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8025EFF4 - the act's motion/act hand-off the rig updater `Pl/fn_80224AC4.cpp` tests. */
u32 fn_8025EFF4(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_80258FCC_H */
