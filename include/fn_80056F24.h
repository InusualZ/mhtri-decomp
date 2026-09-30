/* The declarations `src/fn_80056F24.cpp` owns (docs/plan.md 6.5 rule 2).  The unit was written before
 * any other TU called its bodies, so it had no header; this one carries the two the multiplayer-result
 * band drives (`menu/multi_result.cpp`).  Add the rest here as their consumers appear.
 */
#ifndef MHTRI_FN_80056F24_H
#define MHTRI_FN_80056F24_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80058EE8 - arm the system block's copy-filter handshake: it flags +0x869 only while +0x868 is
 * already 2 (i.e. the filter is preparing). */
void system_copy_filter_arm(void);
/* 0x80058F08 - clear both handshake bytes (+0x868/+0x869). */
void system_copy_filter_clear(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80058044 - starts fade `slot` toward table entry `table_index`; the map's `fade_set__Fll`.  Added with
 * `quest/arenatask.cpp` (rule 2: this range owns the address). */
void fade_set(s32 slot, s32 table_index);
#endif

#endif /* MHTRI_FN_80056F24_H */
