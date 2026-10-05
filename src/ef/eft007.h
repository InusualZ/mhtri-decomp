/* The effect unit `ef/eft007.cpp`.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_EF_EFT007_H
#define MHTRI_EF_EFT007_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The part-flag setter wrapper; the C++ consumers (`enemy/em024_ai.cpp`) call it with the work record and
 * the part id, the C ones keep the old-style declaration. */
#ifdef __cplusplus
void eft007_part_set(struct _ENEMY_WORK* enemy, u32 part);
#else
void eft007_part_set();
#endif

/* The real signature, from the owner's own definition (`src/ef/eft007.cpp`:
 * `void eft007_part_spawn(_ENEMY_WORK* enemy, u8 part, s32 pos_x, s32 pos_y)`).  C++ gets it because
 * `enemy/fn_80147CE0.cpp` calls it with four arguments; the C consumers keep the old-style
 * declaration (they call it with four arguments too, which C allows).  One view per TU: declaring
 * both spellings is `(10197) illegal function overloading`. */
#ifdef __cplusplus
void eft007_part_spawn(struct _ENEMY_WORK* enemy, u8 part, s32 pos_x, s32 pos_y);
#else
void eft007_part_spawn();
#endif


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_80101FA4(void* self);
void fn_801025E8(void* self);
void fn_801025F8(void* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT007_H */
