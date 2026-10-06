/* Declarations owned by `Pl/pl_act_step.cpp` (rule 2). */
#ifndef MHTRI_PL_ACT_STEP_H
#define MHTRI_PL_ACT_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80257E70 - the three-timer reset of the guard act (GUESS name: the map had only the stem and the runtime dump a
 * placeholder); `extern "C"` because the map row is unmangled. */
void pl_act_guard_timer_reset(struct _PLW* self);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/Pl.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80245DA0 - the act tail's wait clear, which the act state-machine band calls.  `void`: the body ends without
 * setting r3 and the three call sites drop the result. */
void pl_act_clear_wait(struct _PLW* self, u8 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ACT_STEP_H */
