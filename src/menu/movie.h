/*
 * menu/movie.h - declarations of `menu/movie.cpp` (no bodies yet).  `NetworkFriendInfo` is the record a community friend
 *   entry ends with (`NetworkCommunityFriend::info_38`, `Network/NetworkCommunityPat.h`): its constructor 0x8043ECD8 and
 *   table 0x806048A0 sit in this unit, its only caller is `NetworkCommunityPat`'s friend-entry constructor, and the
 *   table's destructor slot is the inline empty destructor that unit emits (0x803F0534).  The class name is a GUESS.
 */
#ifndef MHTRI_MENU_MOVIE_H
#define MHTRI_MENU_MOVIE_H

#include "types.h"
#include "menu/menu_scroll.h"
#include "Network/net_peer_card.h"

class NetworkFriendInfo {
public:
    NetworkFriendInfo();                  /* 0x8043ECD8 - stores the table, zeroes `data_04` */
    virtual ~NetworkFriendInfo() {}

    /* +0x04 */ u8 data_04[0x48];
};   /* size: 0x4C (the constructor's memset of 0x48 bytes after the table pointer) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043F280 - the +0x0A byte of the movie player's state (.bss 0x806E28A0), which its playback loop 0x8043ED34
 * raises while a movie runs (with screen dimming disabled) and clears after.  NAME (a GUESS): the pad reader, the
 * system core and the network message pool all skip their work while it reads 1. */
BOOL isMoviePlaying(void);

/* 0x8043F088 - waits while the movie player holds the drive (`info` the file a read was just queued on, or NULL);
 * the system core's loaders call it around every disc access. */
struct DVDFileInfo;
void movie_drive_wait(struct DVDFileInfo* info);

/* 0x8043F26C - asks the disc-error code for a disc change. */
void movie_change_request(void);
/* 0x8043ED20 / 0x8043ED34 / 0x8043EF64 - the disc-error screen's clear, per-frame step and draw (the system task
 * table's entries). */
void dvd_error_init(void);
void dvd_error_step(void);
void dvd_error_draw(void);
/* 0x8043F290 - blanks the video and stops the audio and controller callbacks before a shutdown or restart. */
void system_shutdown_prepare(void);

/* 0x8043E484 - the open movie's frame count (the THPSimple sample's). */
s32 THPSimpleGetTotalFrame(void);

/* The friend picker (its `menu_friendlist.cpp` run from 0x8043F314), called by `menu/menu_plsearch.cpp`'s message
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

/* 0x8043D868 - 1 while a movie is on screen (C++ scope: `movie_playing_ck__Fv`). */
u8 movie_playing_ck(void);
/* 0x8043D828 - closes the movie once its first frame is up; 1 when it closed (C++ scope: `Movie_cut__Fv`). */
BOOL Movie_cut(void);
#endif

#endif /* MHTRI_MENU_MOVIE_H */
