/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_stick_dir_ck`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_STICK_DIR_CK_H
#define MHTRI_PL_PL_STICK_DIR_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026FB20 - 1 when the stick points into the angle window of a direction code (registry name). */
u32 pl_stick_dir_ck(struct _PLW* self, u32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_STICK_DIR_CK_H */
