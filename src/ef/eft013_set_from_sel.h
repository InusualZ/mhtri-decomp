/* ef/eft013_set_from_sel.h - leaf header: `eft013_set_from_sel` (0x801073B8), owned by `ef/eft013_fx.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT013_SET_FROM_SEL_H
#define MHTRI_EF_EFT013_SET_FROM_SEL_H

#include "types.h"
#include "nw4r/math.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the eft013 effect of `type` at `pos` for the hit's target record. */
void eft013_set_from_sel(struct _HIT_W* hit, VEC3* pos, u8 type);
#ifdef __cplusplus
}
#endif

#endif
