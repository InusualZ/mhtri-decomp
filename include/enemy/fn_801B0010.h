/* The enemy unit `enemy/fn_801B0010.cpp` (0x801B0010..0x801B4458): the em030 (enemy #30) program.
 *
 * Declarations for the symbols of that range OTHER units call (docs/plan.md 6.5 rule 2: an extern
 * lives with the TU that owns the symbol).  They moved here with the unit's registration:
 *
 *   * `fn_801B0010` was parked in `include/unsplit/enemy.h` with the note "owned by the
 *     still-unregistered proposal/801B0010 range" - the band header is for symbols with no owner, so
 *     the declaration left it, and `enemy/fn_801A9540.cpp` (its caller) includes this header now.
 *   * `fn_801B4348` / `fn_801B4398` were declared in `enemy/fn_801B4458.cpp`'s own callee block,
 *     which is the same boundary artefact one step further down the band; that unit includes this
 *     header now.  Signatures are unchanged (they are the owner's own definitions').
 */
#ifndef MHTRI_ENEMY_FN_801B0010_H
#define MHTRI_ENEMY_FN_801B0010_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801B0010 - the area predicate `enemy/fn_801A9540.cpp` calls: r3 the area and the answer in r3
 * (its caller compares it against 1).  The owner's own definition spells the parameter `u32 area`
 * (the body narrows it with `(u8)area` itself); the band header's `u8 area` was the call site's
 * reading of the same register. */
u32 fn_801B0010(u32 area);
/* 0x801B4348 - the seat/state reset `enemy/fn_801B4458.cpp` runs at its two teardown steps. */
void fn_801B4348(struct _ENEMY_WORK* self);
/* 0x801B4398 - the seat lookup that band walks: r3 the work record, r4 the kind set, r5 the `u32*`
 * out record; the answer is the record count. */
u32 fn_801B4398(struct _ENEMY_WORK* self, u32 kind, u32* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801B0010_H */
