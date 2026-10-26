/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `Pl/pl_act.cpp` defines, for a consumer that does not include the
 * owner's header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_PL_INPUT_LOCK_SET_H
#define MHTRI_PL_PL_INPUT_LOCK_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026AE04 - stores `value` in the local player's move-work countdown at +0x352. */
void pl_input_lock_set(s16 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_INPUT_LOCK_SET_H */
