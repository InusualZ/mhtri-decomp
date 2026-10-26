/* `place_info_set`, defined by `menu/menu_placeinfo.cpp` (leaf header, docs/plan.md 6.5 rule 2).  C linkage (the map
 * row is a plain name). */
#ifndef MHTRI_MENU_PLACE_INFO_SET_H
#define MHTRI_MENU_PLACE_INFO_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Shows the placed models of a room's place info (0x11 interior ids; NULL clears it). */
void place_info_set(u8* info);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_INFO_SET_H */
