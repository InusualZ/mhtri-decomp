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

/* 0x80058BB0 - request the copy filter: +0x868 becomes 1 unless it already is (the handshake's first
 * step; `system_copy_filter_arm` follows it).  GUESS name. */
void system_copy_filter_request(void);
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
/* 0x800584B0 - resets the screen copy filter (`filter_reset__Fv`; the owner defines it at C++ scope,
 * src/fn_80056F24.cpp:436).  Added with `quest/arenatask.cpp`. */
void filter_reset(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80058008 - turns the filter panel on. */
void filter_panel_on(void);
/* 0x800594C8 / 0x8005801C - clear the filter's +0x13C flag and its panel's first word (GUESS names). */
void filter_flag_clear(void);
void filter_panel_flag_clear(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800578A0 - turns the glare filter on (C++ scope: the map row is `GlareFilter_on__Fv`). */
void GlareFilter_on(void);
#endif

#ifdef __cplusplus
/* 0x800580EC - resets fade slot `slot` (C++ scope: `fade_reset__Fl`). */
void fade_reset(long slot);
#endif

#endif /* MHTRI_FN_80056F24_H */
