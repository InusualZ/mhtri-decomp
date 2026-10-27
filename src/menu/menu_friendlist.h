/*
 * menu/menu_friendlist.h - declarations of `menu/menu_friendlist.cpp`: the friend picker's work and entry points.
 */
#ifndef MHTRI_MENU_MENU_FRIENDLIST_H
#define MHTRI_MENU_MENU_FRIENDLIST_H

#include "types.h"
#include "menu/menu_scroll.h"
#include "Network/net_peer_card.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The friend picker (from 0x8043F314), called by `menu/menu_plsearch.cpp`'s message
 * menu.  GUESS names, from the bodies and their one caller. */
struct LobbyTimerRow;
struct LobbyFriendList;
/* The picker's work: its steps, its mode, the pad word, the page scroll over the lobby's friend list, the row the
 * player picked, the eight hunter cards of the page, and the friend-request state.
 * size: 0x10D0 (the clear `friend_pick_init` runs) */
typedef struct FriendPickWork {
    /* +0x000 */ u8 step_0x000;
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u8 request_step_0x002;      /* the friend request's sub-step (step 6) */
    /* +0x003 */ u8 mode_0x003;              /* 1: return the picked row, else open its card */
    /* +0x004 */ u16 input_0x004;            /* the pad's pressed word plus the held d-pad bits */
    /* +0x006 */ u16 moved_0x006;            /* the scroll's move flags of the frame */
    /* +0x008 */ struct MenuScroll scroll_0x008;
    /* +0x014 */ struct LobbyFriendList* list_0x014;
    /* +0x018 */ struct LobbyTimerRow* picked_0x018;   /* the friend row the player chose */
    /* +0x01C */ NetPeerCard cards_0x01C[8];  /* the page's cards, fetched one by one */
    /* +0x10AC */ u8 page_top_0x10AC;        /* the list index of the page's first row */
    /* +0x10AD */ u8 fetched_0x10AD;         /* the cards fetched so far */
    /* +0x10AE */ s8 result_0x10AE;          /* the pending network request's result (0 while pending) */
    /* +0x10AF */ u8 pad_0x10AF;
    /* +0x10B0 */ s16 timer_0x10B0;          /* frames the leave message stays up */
    /* +0x10B2 */ char id_0x10B2[10];        /* the picked card's id text */
    /* +0x10BC */ char name_0x10BC[0x14];    /* the picked card's name */
} FriendPickWork; /* size: 0x10D0 */

/* 0x8043F314 (0x78): clears `work`, stores `mode` and points it at the lobby's friend list. */
void friend_pick_init(FriendPickWork* work, u8 mode);
/* 0x8043F398 (0x6CC): steps the picker: 1 next, 2 back, 3 or 4 done. */
s32 friend_pick_step(FriendPickWork* work);
/* 0x8043FA70 (0x9C): draws the picker. */
void friend_pick_draw(FriendPickWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRIENDLIST_H */
