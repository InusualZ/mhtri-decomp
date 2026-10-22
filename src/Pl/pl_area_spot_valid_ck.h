/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_area_spot_valid_ck`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_PL_PL_AREA_SPOT_VALID_CK_H
#define MHTRI_PL_PL_AREA_SPOT_VALID_CK_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80278310 (0x140): whether the spot in the area is usable on the current map */
u32 pl_area_spot_valid_ck(u8 area, nw4r::math::VEC3* pos, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_AREA_SPOT_VALID_CK_H */
