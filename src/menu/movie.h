/*
 * menu/movie.h - declarations of `menu/movie.cpp` (no bodies yet).  `NetworkFriendInfo` is the record a community friend
 *   entry ends with (`NetworkCommunityFriend::info_38`, `Network/NetworkCommunityPat.h`): its constructor 0x8043ECD8 and
 *   table 0x806048A0 sit in this unit, its only caller is `NetworkCommunityPat`'s friend-entry constructor, and the
 *   table's destructor slot is the inline empty destructor that unit emits (0x803F0534).  The class name is a GUESS.
 */
#ifndef MHTRI_MENU_MOVIE_H
#define MHTRI_MENU_MOVIE_H

#include "types.h"

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

/* The friend picker (its `menu_friendlist.cpp` run from 0x8043F314), called by `menu/menu_plsearch.cpp`'s message
 * menu.  GUESS names, from the bodies and their one caller. */
struct LobbyTimerRow;
/* The picker's work: its step bytes, its mode, the list it shows and the row the player picked.
 * size: 0x10D0 (the clear `friend_pick_init` runs) */
typedef struct FriendPickWork {
    /* +0x000 */ u8 step_0x000;
    /* +0x001 */ u8 sub_0x001;
    /* +0x002 */ u8 pad_0x002;
    /* +0x003 */ u8 mode_0x003;
    /* +0x004 */ u8 pad_0x004[0x14];
    /* +0x018 */ struct LobbyTimerRow* picked_0x018;   /* the friend row the player chose */
    /* +0x01C */ u8 pad_0x01C[0x10B4];
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

#endif /* MHTRI_MENU_MOVIE_H */
