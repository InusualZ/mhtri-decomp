/*
 * Declarations owned by `Pl/fn_80241558.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.
 */
#ifndef MHTRI_PL_FN_80241558_H
#define MHTRI_PL_FN_80241558_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80241558 - the motion bank for kind 5; the owner defines it `extern "C"` (its map name is
 * unmangled).  Added with `Pl/fn_80230FBC.cpp`, the kind dispatch that calls it. */
void fn_80241558(struct _PLW* work, u8 part);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_80241558_H */
