/* ef/eft025_pl_set.h - leaf header: `eft025_pl_set` (0x80117688), owned by `ef/fn_801173AC.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT025_PL_SET_H
#define MHTRI_EF_EFT025_PL_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the eft025 effect of `kind` for `plw` when it stands in the current area. */
void eft025_pl_set(struct _PLW* plw, u32 kind);
#ifdef __cplusplus
}
#endif

#endif
