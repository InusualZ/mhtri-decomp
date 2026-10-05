/*
 * Declarations for the symbols `src/ef/fn_8010BDE4.cpp` owns (docs/plan.md 6.5, rule 2).  The signatures
 * are the owner's own `extern "C"` definitions, so a consumer that includes this header cannot disagree
 * with the owner.  The only consumer today is `ef/fn_80105314.cpp`'s `fn_8010BDA8` dispatcher.
 */
#ifndef MHTRI_EF_FN_8010BDE4_H
#define MHTRI_EF_FN_8010BDE4_H

#include "types.h"

struct _EFT;

#ifdef __cplusplus
extern "C" {
#endif

/* Frame family: seed the two per-model records, then place them along their angles. */
void fn_8010BDE4(_EFT* self);
void fn_8010C0E0(_EFT* self);
/* Light family: the two state hooks the dispatcher routes to. */
void fn_8010C454(_EFT* self);
void fn_8010C464(_EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_8010BDE4_H */
