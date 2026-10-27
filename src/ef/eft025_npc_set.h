/* `eft025_npc_set`, defined by `ef/fn_801173AC.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EF_EFT025_NPC_SET_H
#define MHTRI_EF_EFT025_NPC_SET_H

#include "types.h"
#include "nw4r/math.h"

struct _AINPC_W;

#ifdef __cplusplus
extern "C" {
#endif

/* Starts the eft025 kind-3 effect on the NPC (in its area only) with scale `scale`. */
void eft025_npc_set(struct _AINPC_W* npc, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT025_NPC_SET_H */
