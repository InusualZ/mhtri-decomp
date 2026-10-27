/* ef/fn_8010BDE4.h - the declarations of `ef/fn_8010BDE4.cpp`'s symbols, in the owner's own `extern "C"` signatures
 * (docs/plan.md 6.5 rule 2); `ef/eft013_fx.cpp`'s `fn_8010BDA8` dispatcher calls them. */
#ifndef MHTRI_EF_FN_8010BDE4_H
#define MHTRI_EF_FN_8010BDE4_H

#include "types.h"
#include "ef/eft016.h" /* eft_pl_hit_light_spawn, for the units that still include this header for it */

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
