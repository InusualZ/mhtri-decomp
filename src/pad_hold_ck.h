/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_PAD_HOLD_CK_H
#define MHTRI_PAD_HOLD_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80046F0C - 1 while the pad hold gate is up. */
u32 pad_hold_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PAD_HOLD_CK_H */
