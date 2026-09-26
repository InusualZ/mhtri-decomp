/*
 * Declarations owned by `Pl/fn_8024F200.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.
 */
#ifndef MHTRI_PL_FN_8024F200_H
#define MHTRI_PL_FN_8024F200_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80257E70 - the three-timer reset `Pl/fn_8024F200.cpp` defines at 0x80257E70; the map name is
 * unmangled, so the definition is `extern "C"` and so is this declaration. */
void fn_80257E70(struct _PLW* self);

/* 0x8025FA00 - this unit's range owns the address, but the body is not reconstructed yet; the
 * signature is the one the previous band header carried.  It is declared here anyway so the
 * declaration is not left in `include/unsplit/Pl.h` (rule 2 forbids the band header holding a
 * symbol a registered unit owns). */
u32 fn_8025FA00(void* a, void* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_8024F200_H */
