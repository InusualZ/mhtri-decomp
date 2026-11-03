#ifndef MHTRI_EF_FN_80114E34_H
#define MHTRI_EF_FN_80114E34_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_80114E34.cpp` owns (docs/plan.md 6.5, rule 2; playbook 51).
 * Kept minimal - only the declarations a consumer needs - and C-visible, because these are plain
 * C-linkage symbols (`fn_XXXXXXXX`).  Where the owner and a consumer view the object through
 * different local types, the parameter is `void*` so one header serves them all without a guess.
 */
#ifdef __cplusplus
extern "C" {
#endif

void fn_80116FCC(void* self);
void fn_80117074(void* self);

/* The eft020 creator the actor spawners call: creates the family-20 record (`_EFT` from `ef.h`) for
 * `(type, area)` and returns it, or 0 when the area check fails. */
struct _EFT;
struct _EFT* fn_80114E34(u8 type, u8 area, f32 scale);

/* 0x80114C20 - spawns the eft020 record keyed `key` on an actor, in the actor's area.  `Pl/pl_act.cpp`,
 * `Pl/pl_act_step.cpp` and `lobby/lb_npc.cpp` pass the player or NPC work. */
/* untyped: caller-owned payload, the spawning actor's work record the effect keeps as its source */
void ef_inst_spawn(void* actor, u8 key);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_80114E34_H */
