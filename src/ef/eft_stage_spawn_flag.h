/* ef/eft_stage_spawn_flag.h - leaf header: `eft_stage_spawn_flag` (0x800F9380), owned by `ef/eft_res.cpp`. The owner
 * defines it without a parameter (its body never reads r3); the callers pass the player. */
#ifndef MHTRI_EF_EFT_STAGE_SPAWN_FLAG_H
#define MHTRI_EF_EFT_STAGE_SPAWN_FLAG_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Maps the stage's effect size class to the spawn flag. */
u32 eft_stage_spawn_flag(struct _PLW* pl);
#ifdef __cplusplus
}
#endif

#endif
