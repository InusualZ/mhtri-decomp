/*
 * Declarations owned by `Pl/pl_act_step.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.
 */
#ifndef MHTRI_PL_ACT_STEP_H
#define MHTRI_PL_ACT_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80257E70 - the three-timer reset of the guard act (GUESS, class 4: the map has only the `fn_`
 * stem and `dumpmap.py` answers `zz_0257e70_`); its definition is `extern "C"` because the map row
 * is unmangled.  `Pl/fn_80258FCC.cpp` is its only caller outside this unit. */
void pl_act_guard_timer_reset(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ACT_STEP_H */
