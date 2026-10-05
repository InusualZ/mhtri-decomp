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

/* Declarations moved here from `unsplit/Pl.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The act tail's predicates and setters, called by `Pl/fn_80258FCC.cpp` (`.text`
 * 0x80258FCC-0x8025F088).  `pl_act_clear_wait` sits in the unclaimed run 0x802430E8-0x80258FCC, so the band
 * header is its rule-2 home; `pl_act_set_cam_ang` sits inside `Pl/fn_802693C4.cpp`'s range and is declared
 * in `Pl/fn_802693C4.h`. */
/* The return was `u32`; the owner's own body ends without ever setting r3 (the value callers would
 * read is `Pl_master_ck`'s), and `Pl/fn_80258FCC.cpp` drops it at all three call sites, so the
 * declaration is `void` (docs/plan.md 6.5 rule 2: the owner owns the spelling). */
void pl_act_clear_wait(struct _PLW* self, u8 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ACT_STEP_H */
