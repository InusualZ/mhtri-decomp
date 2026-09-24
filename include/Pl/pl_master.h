/* The player master unit `Pl/pl_master.cpp`.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_PL_PL_MASTER_H
#define MHTRI_PL_PL_MASTER_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8026FE98(struct _ENEMY_WORK* other, u32 mask);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
struct _PLW;
u32 Pl_master_ck(struct _PLW* plw);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MASTER_H */
