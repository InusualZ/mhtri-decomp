/* menu/menu_friend_list_init.h - leaf header (docs/plan.md 6.5 rule 2): the hunter-card view's init/step/draw
 *   `menu/movie.cpp` defines, for `menu/menu_plsearch.cpp`.  C linkage (the map rows are plain
 *   names).  GUESS names. */
#ifndef MHTRI_MENU_MENU_FRIEND_LIST_INIT_H
#define MHTRI_MENU_MENU_FRIEND_LIST_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FriendCardView;   /* the 0x148-byte card view; its layout stays with `menu/movie.cpp` */
struct NetPeerCard;      /* Network/net_session_close.h */

/* 0x8043FF2C / 0x80440268 / 0x80440690 - opens the view on `card` (`mode`, `flag`), steps it (0 busy, 2 send a
 * request, 3 back, else closed) and draws it. */
void menu_friend_list_init(struct FriendCardView* view, struct NetPeerCard* card, u8 mode, u8 flag);
s32 menu_friend_list_step(struct FriendCardView* view);
void menu_friend_list_draw(struct FriendCardView* view);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRIEND_LIST_INIT_H */
