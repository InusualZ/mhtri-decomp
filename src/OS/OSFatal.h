/*
 * OS/OSFatal.h - declarations of the symbols owned by `OS/OSFatal.c` that other units call.
 */
#ifndef OS_OSFATAL_H
#define OS_OSFATAL_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CF7A0 - stops the system, shows `message` on a solid `bg` screen in `fg` text and halts. */
void OSFatal(GXColor fg, GXColor bg, const char* message);

#ifdef __cplusplus
}
#endif

#endif
