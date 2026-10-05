/*
 * Declarations of `src/menu/movie.cpp` (`.text` 0x8043D524..0x804459DC; the unit has no bodies yet).
 *
 * `NetworkFriendInfo` is the record a community friend entry ends with (`NetworkCommunityFriend::info_38`,
 * include/Network/NetworkCommunityPat.h).  Its constructor 0x8043ECD8 and its table 0x806048A0 sit in this unit's
 * ranges; its only caller is `NetworkCommunityPat`'s friend-entry constructor, and the table's destructor slot is the
 * inline empty destructor that unit emits (0x803F0534).  The class name is a GUESS from the friend entry it ends.
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

#endif /* MHTRI_MENU_MOVIE_H */
