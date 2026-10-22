/* menu/get_option_pair.h - leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s `get_option_pair`, for
 *   `menu/menu_placeinfo.cpp`.  C linkage (the map row is a plain name); the signature is the owner's definition. */
#ifndef MHTRI_MENU_GET_OPTION_PAIR_H
#define MHTRI_MENU_GET_OPTION_PAIR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BEE38: the two bytes of entry `index` of the option pair table. */
void get_option_pair(u8 index, u8* value, u8* flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_GET_OPTION_PAIR_H */
