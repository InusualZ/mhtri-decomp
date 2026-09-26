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
/* 0x8026FE44 - the second master predicate `Pl/fn_802489D4.cpp` gates a motion on.  The map name is
 * the bare `fn_8026FE44`, so the owner defines it `extern "C"` and so is the declaration. */
u32 fn_8026FE44(struct _PLW* self);
/* 0x8026F908 - this unit's own helper `Pl/fn_802489D4.cpp` calls; the map name is a bare `fn_` stem,
 * so `extern "C"`. */
u8 fn_8026F908(struct _PLW* self, u32 idx);
/* 0x8026FEC0 - the status-bit setter the player act cluster (`Pl/fn_802489D4.cpp`) and the player
 * act dispatcher (`Pl/fn_8024F200.cpp`) drive; the owner defines it unmangled (`Pl/pl_master.cpp`),
 * so the declaration is `extern "C"`.  One declaration: `mask`/`bits` are the same `u32`. */
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

/* Declarations added with `Pl/fn_80258FCC.cpp` (the act state-machine band): added at this header's
 * C++ scope, i.e. the same linkage as the symbols above.  `fn_8026FEC0` is declared (once, with
 * `extern "C"`) in the status-bit setter block near the top - MAIN's copy is the authority for it. */
void fn_8026FEF0(struct _PLW* self, s32 v);
u32 fn_8026FE44(struct _PLW* self);
u8 fn_8026F908(struct _PLW* self, u32 a);
u32 fn_8026F888(struct _PLW* self);
/* 0x8026FB20 - the act's status-bit test `Pl/fn_80273B14.cpp`'s act entry gates its
 * direction flip on; added with that unit (rule 2). */
u32 fn_8026FB20(struct _PLW* self, u32 mask);
#endif

#endif /* MHTRI_PL_PL_MASTER_H */
