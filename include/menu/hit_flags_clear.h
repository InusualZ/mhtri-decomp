/* Leaf header for two hit-record helpers `menu/menu_item.cpp` owns (0x8029F538, 0x8029F5B4): the owner's
 * full header redefines `_HIT_W` and pulls `pl.h`, so a consumer that has its own `_HIT_W` view includes
 * this instead (rule 2).  They clear a hit record and set its knock-back value.
 */
struct _HIT_W;

#ifndef MHTRI_MENU_HIT_FLAGS_CLEAR_H
#define MHTRI_MENU_HIT_FLAGS_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8029F538 - clears the hit record. */
void hit_flags_clear(struct _HIT_W* hit);
/* 0x8029F5B4 - stores `value` (widened to a float) as the hit's second float. */
void hit_knock_set(struct _HIT_W* hit, s16 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_FLAGS_CLEAR_H */
