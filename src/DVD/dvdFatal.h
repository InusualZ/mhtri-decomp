/*
 * DVD/dvdFatal.h - declarations of the symbols owned by `DVD/dvdFatal.c` that other units call.
 */
#ifndef DVD_DVDFATAL_H
#define DVD_DVDFATAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804ABDD0 (0xCC): shows the region's and language's "disc error" message through OSFatal. */
void __DVDShowFatalMessage(void);

/* 0x804ABEA0 (0x5C): installs or removes the automatic fatal message; returns whether it was installed. */
BOOL DVDSetAutoFatalMessaging(BOOL enable);

/* 0x804ABF00 (0x14): returns whether the automatic fatal message is installed. */
BOOL __DVDGetAutoFatalMessaging(void);

/* 0x804ABF20 (0x18): calls the installed fatal-message handler, if any. */
void __DVDPrintFatalMessage(void);

#ifdef __cplusplus
}
#endif

#endif
