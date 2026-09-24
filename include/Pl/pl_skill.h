/* The player skill unit `Pl/pl_skill.cpp`.
 *
 * `Pl_Skill_ck` is a C++ free function (the map's name `Pl_Skill_ck__FP4_PLWUs` is its mangling),
 * moved here from `enemy/fn_8012BDF4.cpp` (docs/plan.md 6.5 rule 2).  A C++ consumer calls it through
 * the owner's real declaration, which the front-end mangles back to the map's name; a C consumer
 * cannot name the mangling, so it gets the map's spelling under `extern "C"` (rule 9's C limitation).
 */
#ifndef MHTRI_PL_PL_SKILL_H
#define MHTRI_PL_PL_SKILL_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
u32 Pl_Skill_ck(struct _PLW* work, u16 skill); /* -> Pl_Skill_ck__FP4_PLWUs */
#else
u32 Pl_Skill_ck__FP4_PLWUs(struct _PLW* work, u16 skill);
#endif

#endif /* MHTRI_PL_PL_SKILL_H */
