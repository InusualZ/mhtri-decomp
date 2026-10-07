/* ef/eft002.h - the declarations of `ef/eft002.cpp`'s state-machine targets and its player gate (docs/plan.md 6.5
 * rule 2). */
#ifndef MHTRI_EF_EFT002_H
#define MHTRI_EF_EFT002_H

#include "types.h"

struct _EFT;
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The state machine the pooled effect runs: the per-frame body, its `byte_5` bump, the destructor
 * thunk and the shell-type gate. */
void fn_800FCED4(struct _EFT* self);
void fn_800FD29C(struct _EFT* self);
void fn_800FD2AC(struct _EFT* self);
u32 fn_800FD2B0(struct _PLW* self);

#ifdef __cplusplus
}

namespace nw4r {
namespace ef {
struct Effect;
}
}

/* 0x800FCEB0 - walks the effect's particle managers with the scale callback (`arg` is the scale pair). */
extern "C" void eft_effect_foreach_pm_scale(nw4r::ef::Effect* self, u32 arg, bool flag);
#endif

#endif /* MHTRI_EF_EFT002_H */
