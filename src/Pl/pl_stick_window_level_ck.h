/* Leaf header (docs/plan.md 6.5 rule 2): `pl_stick_window_level_ck` (0x8026FC40), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_STICK_WINDOW_LEVEL_CK_H
#define MHTRI_PL_PL_STICK_WINDOW_LEVEL_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

u32 pl_stick_window_level_ck(struct _PLW* self, u32 idx, u16 low, u16 high);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_STICK_WINDOW_LEVEL_CK_H */
