/*
 * KBD/kbd.h - declarations of the symbols owned by `KBD/kbd.c` that other units call.
 */
#ifndef KBD_KBD_H
#define KBD_KBD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80529430 - allocate an LED request for the channel, store the value, the callback and its argument,
 * and hand it to the HID transfer; 7 when the request cannot be queued. */
/* untyped: opaque band object, typed by the callers' views */
u32 KBDSetLedsAsync(u32 index, u32 value, void* callback, u32 arg);

/* 0x80529B50 - store the word at +0x258 of the channel's 0x2A8-byte record; returns 0. */
s32 KBDSetChannelValue(u8 index, u32 value);

#ifdef __cplusplus
}
#endif

#endif
