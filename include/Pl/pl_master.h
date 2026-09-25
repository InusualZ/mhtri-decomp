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
#ifdef __cplusplus
}
#endif

/* 0x8026FD94 - the master action predicate `Pl/fn_80229ECC.cpp` gates on; the owner defines it
 * `extern "C"` (its map name is unmangled). */
#ifdef __cplusplus
extern "C" {
#endif
struct _PLW;
u32 fn_8026FD94(struct _PLW* self);
#ifdef __cplusplus
}
#endif

/* `Pl/pl_master.cpp` defines it at C++ scope and the target object references
 * `Pl_master_ck__FP4_PLW`, so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
u32 Pl_master_ck(struct _PLW* plw);
#endif

#endif /* MHTRI_PL_PL_MASTER_H */
