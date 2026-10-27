/* ef/eft013_shell_spawn.h - leaf header: `eft013_shell_spawn` (0x80106BB4), owned by `ef/fn_80105314.cpp`. C linkage. */
#ifndef MHTRI_EF_EFT013_SHELL_SPAWN_H
#define MHTRI_EF_EFT013_SHELL_SPAWN_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

struct _HIT_W;

#ifdef __cplusplus
extern "C" {
#endif
/* Spawns the eft013 hit effect of `type` at `pos` turned by `rot`, scaled by `scale`, for the hit's owner. */
void eft013_shell_spawn(VEC3* pos, u32 type, _CP_VECTOR* rot, f32 scale, struct _HIT_W* src);
#ifdef __cplusplus
}
#endif

#endif
