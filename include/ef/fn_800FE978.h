#ifndef MHTRI_EF_FN_800FE978_H
#define MHTRI_EF_FN_800FE978_H

#include "types.h"

/* The spawn/init state machine `src/ef/fn_800FE978.cpp` owns (docs/plan.md 6.5 rule 2).  Its one caller
 * outside the owner is `ef/fn_800FD864.cpp`, which passes the 0x48-byte effect slot, so the declaration
 * uses that view (`struct _EFT` is `ef.h`'s, and the owner defines it against the same tag).
 */
struct _EFT;
void fn_800FE978(struct _EFT* self);

#endif /* MHTRI_EF_FN_800FE978_H */
