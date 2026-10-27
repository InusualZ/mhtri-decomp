/* Leaf header (docs/plan.md 6.5 rule 2): `pl_reload_needed_ck` (0x802798FC), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_RELOAD_NEEDED_CK_H
#define MHTRI_PL_PL_RELOAD_NEEDED_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

s32 pl_reload_needed_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_RELOAD_NEEDED_CK_H */
