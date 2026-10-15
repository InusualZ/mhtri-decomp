/*
 * KBD/kbd.h - declarations of the symbols owned by `KBD/kbd.c` that other units call.
 */
#ifndef KBD_KBD_H
#define KBD_KBD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct KBDState;

/* Connect callback: the channel record and the driver state. */
typedef void (*KBDChannelCallback)(const u8* channel, struct KBDState* state);

/* Disconnect callback: the channel record. */
typedef void (*KBDDisconnectCallback)(const u8* channel);

/* Key callback: a key event is queued. */
typedef void (*KBDKeyEventCallback)(void);

/* 0x80529430 - allocate an LED request for the channel, store the value, the callback and its argument,
 * and hand it to the HID transfer; 7 when the request cannot be queued. */
/* untyped: opaque band object, typed by the callers' views */
u32 KBDSetLedsAsync(u32 index, u32 value, void* callback, u32 arg);

/* 0x80529B50 - store the word at +0x258 of the channel's 0x2A8-byte record; returns 0. */
s32 KBDSetChannelValue(u8 index, u32 value);

/* Fills in the key map of layout `index`: its tables, the shift/alt/ctrl columns, the key count and the remap pairs. */
void kbdInitMap(s32 index, u8 stride, u8 lockMask, const u16* keys, const u16* modifiers, u8 shiftColumn,
                u8 altColumn, u8 ctrlColumn, u8 keyCount, s32 layout);

/* Installs the international layout as map 0. */
void kbdInitMapIntl(void);

/* Starts the driver in `memory` with its connect, disconnect and key callbacks. */
s32 KBDInit(struct KBDState* memory, KBDChannelCallback onConnect, KBDDisconnectCallback onDisconnect,
            KBDKeyEventCallback onKey);

/* Stops the driver. */
s32 KBDEnd(void);

/* Installs a new connect callback; returns the previous one. */
KBDChannelCallback KBDSetConnectCallback(KBDChannelCallback callback);

/* Installs a new disconnect callback; returns the previous one. */
KBDDisconnectCallback KBDSetDisconnectCallback(KBDDisconnectCallback callback);

/* Installs a new key callback; returns the previous one. */
KBDKeyEventCallback KBDSetKeyCallback(KBDKeyEventCallback callback);

/* Selects key map `mapIndex` for the channel. */
s32 KBDSetChannelMap(u8 channel, s32 mapIndex);

/* Sets the channel's LED word. */
s32 KBDSetChannelLeds(u8 channel, u32 value);

/* Returns the character of HID usage `usage` under the lock state in map `mapIndex`. */
u16 kbd_lookup_key(u8 usage, s32 lockState, s32 mapIndex);

#ifdef __cplusplus
}
#endif

#endif
