/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_page.cpp`'s `lb_menu_page_step`, the draw callback the lobby
 * screens queue on the sub-transfer list.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_MENU_PAGE_STEP_H
#define MHTRI_LOBBY_LB_MENU_PAGE_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8036640C - draws the open lobby screen's page. */
void lb_menu_page_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_PAGE_STEP_H */
