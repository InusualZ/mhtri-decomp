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

#ifdef __cplusplus
extern "C" {
#endif

void fn_80103960();
void fn_801039B0();


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_80101FA4(void* self);
void fn_801025E8(void* self);
void fn_801025F8(void* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT007_H */
