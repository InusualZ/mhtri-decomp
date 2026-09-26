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
/* 0x8026FEC0 - the status-bit setter the player act cluster drives; the owner defines it unmangled
 * (`Pl/pl_master.cpp`), so the declaration is `extern "C"`. */
void fn_8026FEC0(struct _PLW* self, u32 bits);
#ifdef __cplusplus
}
#endif

/* `Pl/pl_master.cpp` defines it at C++ scope and the target object references
 * `Pl_master_ck__FP4_PLW`, so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
u32 Pl_master_ck(struct _PLW* plw);

/* Declarations added with `Pl/fn_80262940.cpp` (the 0x80262940 player main/control cluster): the two
 * action-state helpers that unit calls, both owned by this unit (0x8026FD0C / 0x8026FE68). */
u32 fn_8026FD0C(struct _PLW* self);
u32 Pl_act_ck(struct _PLW* self, u8 group, u16 action);


#endif

#endif /* MHTRI_PL_PL_MASTER_H */
