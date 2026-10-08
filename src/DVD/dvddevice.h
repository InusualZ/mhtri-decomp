/*
 * DVD/dvddevice.h - declarations of the symbols owned by `DVD/dvddevice.c` that other units call.
 */
#ifndef DVD_DVDDEVICE_H
#define DVD_DVDDEVICE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804ABF50 - checks that the drive on this console is the expected device; zero when it is. */
s32 __DVDCheckDevice(void);

#ifdef __cplusplus
}
#endif

#endif
