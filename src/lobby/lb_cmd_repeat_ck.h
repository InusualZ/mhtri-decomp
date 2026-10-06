/* `lobby/lb_npc.cpp`'s command-mask accessors in the view the page units (`lobby/lb_pane_ui.cpp`,
 * `lobby/lb_menu_page.cpp`) call them with; the owner's view is `lobby/lb_cmd_pressed_ck.h`, and the two cannot meet in one
 * TU (illegal overloading). */
#ifndef MHTRI_LOBBY_LB_CMD_REPEAT_CK_H
#define MHTRI_LOBBY_LB_CMD_REPEAT_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 lb_cmd_pressed_ck(s32 mask);
s32 lb_cmd_repeat_ck(s32 what);
u16 lb_cmd_repeat_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CMD_REPEAT_CK_H */
