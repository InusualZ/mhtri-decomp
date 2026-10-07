/*
 * HID/hid.h - declarations of the symbols owned by `HID/hid.c` that other units call, and the records they pass.
 */
#ifndef HID_HID_H
#define HID_HID_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HIDDeviceInfo;
struct HIDClient;

/* Attach callback of a client: `attached` is 1 when the interface appears, 0 when it goes away; a non-zero return
 * claims the interface. */
typedef s32 (*HIDAttachCallback)(struct HIDClient* client, struct HIDDeviceInfo* info, s32 attached);

/* Completion callback of init, end and release: the result and the caller's argument. */
typedef s32 (*HIDDoneCallback)(s32 result, u32 arg);

/* Completion callback of a request: the interface, the result (0 or a negative HID error), the caller's buffer,
 * the transferred length and the caller's argument. */
/* untyped: caller-owned buffer */
typedef void (*HIDResultCallback)(struct HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg);

/* size: 0x1C - the record a library registers to be told about attaching interfaces; only the first two
 * words are touched by HID itself. */
typedef struct HIDClient {
    /* +0x00 */ struct HIDClient* next;
    /* +0x04 */ HIDAttachCallback attach;
    /* +0x08 */ u8 pad_0x08[0x14];
} HIDClient; /* size: 0x1C */

/* size: 0x20 - the part of an interface record handed to clients: id, vendor/product, endpoints and the
 * pointers to its descriptors. */
typedef struct HIDDeviceInfo {
    /* +0x00 */ u32 deviceId;
    /* +0x04 */ u16 vendorId;
    /* +0x06 */ u16 productId;
    /* +0x08 */ u8 interfaceNumber;
    /* +0x09 */ u8 inEndpoint;
    /* +0x0A */ u8 outEndpoint;       /* 0xFF when the interface has none */
    /* +0x0B */ u8 generation;        /* bumped on every attach/detach; a stale request is dropped */
    /* +0x0C */ u8* deviceDescriptor;
    /* +0x10 */ u8* configDescriptor;
    /* +0x14 */ u8* interfaceDescriptor; /* +7 is bInterfaceProtocol */
    /* +0x18 */ u8* inEndpointDescriptor;
    /* +0x1C */ u8* outEndpointDescriptor;
} HIDDeviceInfo; /* size: 0x20 */

/* 0x805272D0 - registers `client` and offers it every interface that is already attached. */
s32 HIDRegisterClient(HIDClient* client, HIDAttachCallback attach);

/* 0x805272E0 - unregisters `client`, detaching its interfaces; `done` runs when none were attached. */
s32 HIDUnregisterClient(HIDClient* client, HIDDoneCallback done, u32 arg);

/* 0x805272F0 - issues the class request SET_REPORT of `type` and `reportId` with `length` bytes of `buffer`. */
/* untyped: report payload */
s32 HIDSetReport(HIDDeviceInfo* info, u8 type, u8 reportId, void* buffer, s32 length, HIDResultCallback done,
                 u32 arg);

/* 0x80527300 - issues the class request SET_PROTOCOL. */
s32 HIDSetProtocol(HIDDeviceInfo* info, s32 protocol, HIDResultCallback done, u32 arg);

/* 0x80527310 - issues the class request SET_IDLE. */
s32 HIDSetIdle(HIDDeviceInfo* info, u8 reportId, u8 duration, HIDResultCallback done, u32 arg);

/* 0x80527320 - reads `length` bytes from the interface's interrupt endpoint into `buffer`. */
/* untyped: report payload */
s32 HIDRead(HIDDeviceInfo* info, void* buffer, s32 length, HIDResultCallback done, u32 arg);
#ifdef __cplusplus
}
#endif

#endif
