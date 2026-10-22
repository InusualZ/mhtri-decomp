/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_message.cpp`'s `menu_pad_map_set`, which
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_PAD_MAP_SET_H
#define MHTRI_MENU_MENU_PAD_MAP_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802AA68C - maps menu pad slot `slot` to `value`. */
void menu_pad_map_set(s32 slot, s32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_PAD_MAP_SET_H */
