/* The player action unit `Pl/pl_act.cpp`: the two condition predicates the enemy units test, plus
 * two plain `fn_` helpers.
 *
 * `Pl_condition_ck`/`Pl_dm_condition_ck` are C++ free functions (the map names are their manglings),
 * moved here from `enemy/fn_8012BDF4.cpp` (docs/plan.md 6.5 rule 2).  A C++ consumer calls the real
 * declaration and the front-end mangles it back to the map's name; a C consumer gets the map's
 * spelling under `extern "C"` (rule 9's C limitation).
 */
#ifndef MHTRI_PL_PL_ACT_H
#define MHTRI_PL_PL_ACT_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8027AC18(void* arg);
u32 fn_8027BC48(s32 arg);

/* 0x8027D050 - the actor's stored carve value, scanned out of the item table.  The owner defines the
 * return `u8`; the retail consumer `Pl/fn_80229ECC.cpp` keeps the raw return in a register and masks
 * it per use (`clrlwi r0,r29,24` before each shift), which MWCC only emits when the declaration is
 * wider than a byte, so the consumer view declared here is 32-bit.  Unmangled (`fn_8027D050`), so the
 * wider return changes no link name. */
u32 fn_8027D050(struct _PLW* self);

#ifdef __cplusplus
}

u32 Pl_condition_ck(struct _PLW* work, u32 condition);    /* -> Pl_condition_ck__FP4_PLWUl */
u32 Pl_dm_condition_ck(struct _PLW* work, u32 condition); /* -> Pl_dm_condition_ck__FP4_PLWUl */
#else
u32 Pl_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
u32 Pl_dm_condition_ck__FP4_PLWUl(struct _PLW* work, u32 condition);
#endif

#endif /* MHTRI_PL_PL_ACT_H */
