/* `eft013_npc_place`, defined by `ef/eft013_fx.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EF_EFT013_NPC_PLACE_H
#define MHTRI_EF_EFT013_NPC_PLACE_H

#include "types.h"
#include "nw4r/math.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif

/* Places an eft013 effect at the NPC (in its area only): offset `pos`, kind `a`, scale `scale`, joint `b`. */
void eft013_npc_place(struct _AINPC_W* self, nw4r::math::VEC3* pos, u32 a, f32 scale, u32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT013_NPC_PLACE_H */
