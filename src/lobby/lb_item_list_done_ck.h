/* lobby/lb_item_list_done_ck.h - leaf: `lobby/lb_menu_pos_tbl.cpp`'s 0x8021F27C (whether `lb_item_list_state`'s +0x3C word is 1) and
 * 0x8021F298 (which clears it). */
#ifndef MHTRI_LOBBY_LB_ITEM_LIST_DONE_CK_H
#define MHTRI_LOBBY_LB_ITEM_LIST_DONE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 lb_item_list_done_ck(void);
void lb_item_list_done_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_LIST_DONE_CK_H */
