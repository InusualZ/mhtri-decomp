/* The declarations `src/ai/fn_802D44F4.cpp` owns (docs/plan.md 6.5 rule 2: a consumer includes the
 * owner's header, it never declares the symbol itself).
 *
 * The unit is the cockpit-side AI band 0x802D44F4-0x802DDC04.  The `fn_802DAxxx` entries below are the
 * ones the cockpit band above it (`menu/fn_802E4978.cpp`, 0x802E4978-0x802E7408) calls; they had been
 * declared in `include/unsplit/menu.h` while the address had no registered owner, and their signatures
 * are that consumer's call sites (no body of them is written yet).
 *
 * `fn_802DA3CC` is a body of this range (0x88 B) whose *address* the consumer registers with
 * `subTransSetPrio`, so it is declared as the function it is rather than as a data object.
 */
#ifndef MHTRI_AI_FN_802D44F4_H
#define MHTRI_AI_FN_802D44F4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_802DA1A8(void* work);            /* 0x802DA1A8 */
void fn_802DA1B0(void* work);            /* 0x802DA1B0 */
void fn_802DA344(void);                  /* 0x802DA344 */
s32 fn_802DA454(s32, s32, s16, s16, s32, s32, s32); /* 0x802DA454 */

void fn_802DA3CC(void);                  /* 0x802DA3CC, registered by address */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_FN_802D44F4_H */
