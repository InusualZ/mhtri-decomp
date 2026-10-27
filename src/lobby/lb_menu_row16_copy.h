/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_menu_row16_copy`.  C linkage (the map row is a
 * plain name). */
#ifndef MHTRI_LOBBY_LB_MENU_ROW16_COPY_H
#define MHTRI_LOBBY_LB_MENU_ROW16_COPY_H

#include "types.h"
#include "lobby/menu_scratch.h" /* `LbMenuRow` */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021DED8 - copies a 0x10-byte menu row field by field. */
void lb_menu_row16_copy(LbMenuRow* dst, const LbMenuRow* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_ROW16_COPY_H */
