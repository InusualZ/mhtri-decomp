/* `lobby/lb_npc.cpp`'s command-mask accessors in the view the page units (`lobby/lb_pane_ui.cpp`,
 * `lobby/lb_menu_page.cpp`) call them with; the owner's view is `lobby/fn_8021213C.h`, and the two cannot meet in one
 * TU (illegal overloading). */
#ifndef MHTRI_LOBBY_FN_802121F4_H
#define MHTRI_LOBBY_FN_802121F4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 fn_8021213C(s32 mask);
s32 fn_802121F4(s32 what);
u16 fn_802122AC(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_FN_802121F4_H */
