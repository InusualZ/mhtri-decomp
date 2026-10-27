/*
 * menu/movie.h - declarations of `menu/movie.cpp`.  `NetworkFriendInfo` is the record a community friend
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

#ifdef __cplusplus
}

/* 0x8043D868 - 1 while a movie is on screen (C++ scope: `movie_playing_ck__Fv`). */
u8 movie_playing_ck(void);
/* 0x8043D828 - closes the movie once its first frame is up; 1 when it closed (C++ scope: `Movie_cut__Fv`). */
BOOL Movie_cut(void);
#endif

#endif /* MHTRI_MENU_MOVIE_H */
