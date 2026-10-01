/* The command-mask accessors of `lobby/lb_npc.cpp` (0x8021213C/0x802121F4/0x802122AC) in the view the lobby page units
 * (`lobby/lb_pane_ui.cpp`, `lobby/lb_menu_page.cpp`) call them with: constant mask arguments, and the `u32`/`s32`
 * returns their compares were written against.  The owner's own view is `lobby/fn_8021213C.h`; the two cannot
 * be visible in one TU (illegal overloading).
 */
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
