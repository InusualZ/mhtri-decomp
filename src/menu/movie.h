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
u8 isMoviePlaying(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MOVIE_H */
