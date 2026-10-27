/* ef/eft013_target_set.h - leaf header: `eft013_target_set` (0x80107914), owned by `ef/eft013_fx.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT013_TARGET_SET_H
#define MHTRI_EF_EFT013_TARGET_SET_H

#include "types.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the eft013 effect on the hit's target record, by the target's kind. */
void eft013_target_set(struct _HIT_W* hit);
#ifdef __cplusplus
}
#endif

#endif
