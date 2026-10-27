/* menu/friend_card_view.h - leaf header (docs/plan.md 6.5 rule 2): the hunter-card view object `menu/menu_friendlist.cpp`'s
 *   `.bss` holds (0x806E3980), for `menu/menu_plsearch.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_MENU_FRIEND_CARD_VIEW_H
#define MHTRI_MENU_FRIEND_CARD_VIEW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FriendCardView;   /* the 0x148-byte card view; its layout stays with `menu/menu_friendlist.cpp` */

/* .bss 0x806E3980 - the card view the player search and the lobby card menu show. */
extern struct FriendCardView friend_card_view;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_FRIEND_CARD_VIEW_H */
