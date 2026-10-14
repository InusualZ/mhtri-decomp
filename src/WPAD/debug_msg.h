/* WPAD/debug_msg.h - the print helper `WPAD/debug_msg.cpp` owns; every WPAD unit logs through it. */
#ifndef WPAD_DEBUG_MSG_H
#define WPAD_DEBUG_MSG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804F9AE0 - takes a log line and discards it. */
void WPADiDebugPrint(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* WPAD_DEBUG_MSG_H */
