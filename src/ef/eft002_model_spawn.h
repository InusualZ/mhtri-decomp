/* ef/eft002_model_spawn.h - leaf header: `eft002_model_spawn` (0x800FC1DC), owned by `ef/eft002.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT002_MODEL_SPAWN_H
#define MHTRI_EF_EFT002_MODEL_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the model effect of `type` at `pos` for the hit (its callers also pass a 0 the body does not read). */
void eft002_model_spawn(VEC3* pos, u32 type, struct _HIT_W* src, u8 unused);
#ifdef __cplusplus
}
#endif

#endif
