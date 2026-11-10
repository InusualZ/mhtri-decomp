#ifndef MHTRI_EF_EFT002_FOLLOW_H
#define MHTRI_EF_EFT002_FOLLOW_H

#include "types.h"

/* Declarations for the symbols `src/ef/fn_800FD718.c` owns (docs/plan.md 6.5, rule 2).  The unit is a
 * C translation unit, so the header is C-visible and the functions carry C linkage.  Kept minimal.
 */
#ifdef __cplusplus
extern "C" {
#endif

struct _EFT;

void eft002_follow(struct _EFT* self);
void eft002_state_next(struct _EFT* self);
void eft002_release(struct _EFT* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT002_FOLLOW_H */
