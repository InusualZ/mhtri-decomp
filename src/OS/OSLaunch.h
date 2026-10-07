/*
 * OS/OSLaunch.h - declarations of the symbols owned by `OS/OSLaunch.c` that other units call or read.
 */
#ifndef OS_OSLAUNCH_H
#define OS_OSLAUNCH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D7ED0 - leaves the game for the Wii Shop Channel's help page: tears down the state events, rebuilds
 * the MEM1 arena, then launches title 00010002-484142xx (`HABA`, or `HABK`/`HABC` when the SC area code 0x804DD2C0 returns 4/5) with the
 * argument "/startup?initpage=showHelp" (.data 0x80629A04).  NAME (a GUESS in the scheme of the
 * `OSLaunchPDChannel` log text beside it); it does not return. */
void OSLaunchShopChannelHelp(void);

/* 0x804D7750 - relaunches the running title with the given reset code. */
void __OSRelaunchTitle(u32 resetCode);

#ifdef __cplusplus
}
#endif

#endif
