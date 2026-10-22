/* `lb_box_menu_close`, defined by `lobby/fn_801E7530.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_BOX_MENU_CLOSE_H
#define MHTRI_LOBBY_LB_BOX_MENU_CLOSE_H

#include "types.h"

struct LbMenuPanel;

#ifdef __cplusplus
extern "C" {
#endif

/* Closes the lobby box menu: clears the screen state, releases the camera and sends a changed profile. */
void lb_box_menu_close(struct LbMenuPanel* menu);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_BOX_MENU_CLOSE_H */
