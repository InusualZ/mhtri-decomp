/* ef/eft016.h - the declarations of `ef/eft016.cpp`'s symbols other units call, in the owner's own `extern "C"`
 * signatures (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT016_H
#define MHTRI_EF_EFT016_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010C554 (0x88): spawns the five-light player hit effect. */
void eft_pl_hit_light_spawn(struct _PLW* source);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT016_H */
