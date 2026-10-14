/* WUD/debug_msg.h - the print helper `WUD/debug_msg.cpp` owns; `WUD/wud.cpp` and `WUD/WUDHidHost.cpp` log through it. */
#ifndef WUD_DEBUG_MSG_H
#define WUD_DEBUG_MSG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80500720 - takes a log line and discards it. */
void WUDiDebugPrint(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* WUD_DEBUG_MSG_H */
