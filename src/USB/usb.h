/*
 * USB/usb.h - the IOS USB client entry points of `USB/usb.cpp` and the request record they share.
 */
#ifndef USB_USB_H
#define USB_USB_H

#include "types.h"

/* Reports the result of an asynchronous open or close; `arg` is the caller's argument. */
typedef void (*USBCallback)(s32 result, void* arg); /* untyped: caller-owned payload */

/* size: 0x0C - an isochronous transfer: the data buffer and the sizes of its packets. */
typedef struct USBIsoRequest {
    /* +0x00 */ void* data; /* untyped: caller-owned transfer buffer */
    /* +0x04 */ u8 packetCount;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u16* packetSizes;
} USBIsoRequest;

/* Reports the result of an asynchronous isochronous transfer with the request it carried. */
typedef void (*USBCallbackEx)(s32 result, USBIsoRequest* iso, void* arg); /* untyped: caller-owned payload */

/* size: 0x80 - one in-flight request: the caller's callbacks, the IPC buffers to free on completion and the device path. */
typedef struct USBRequest {
    /* +0x00 */ USBCallback callback;
    /* +0x04 */ USBCallbackEx callbackEx;
    /* +0x08 */ void* callbackArg; /* untyped: caller-owned payload */
    /* +0x0C */ USBIsoRequest* callbackExtra;
    /* +0x10 */ u8 pad_0x10[4];
    /* +0x14 */ void* clean[7]; /* untyped: IPC heap blocks released when the request completes */
    /* +0x30 */ u8 pad_0x30[4];
    /* +0x34 */ u32 cleanCount;
    /* +0x38 */ u8 pad_0x38[8];
    /* +0x40 */ union {
        char path[0x40];
        struct { /* the buffer and byte count of a transfer request */
            /* +0x40 */ void* data; /* untyped: caller-owned transfer buffer */
            /* +0x44 */ u16 length;
            /* +0x46 */ u8 pad_0x46[2];
        } transfer; /* size: 0x08 */
    };
} USBRequest; /* size: 0x80 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804E46F0 / 0x804E47A0 - the library's trace and error loggers. */
void IUSB_Log(const char* format, ...);
void IUSB_LogError(const char* format, ...);

/* 0x804E4850 / 0x804E4950 - creates the library's IPC heap; closing needs no work. */
s32 IUSB_OpenLib(void);
s32 IUSB_CloseLib(void);

/* 0x804E4960 - completes a request: frees its buffers, runs its callback and frees the request. */
s32 IUSB_RequestDone(s32 result, USBRequest* request);

/* 0x804E4B00 / 0x804E4C50 - opens the device `bus`/`vendor`/`product`, synchronously (descriptor in `fd`) or with a callback. */
s32 IUSB_OpenDeviceIds(const char* bus, s32 vendor, s32 product, s32* fd);
s32 IUSB_OpenDeviceIdsAsync(const char* bus, s32 vendor, s32 product, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

/* 0x804E4F40 - runs a long bulk transfer on `endpoint` of device `fd`; blocks unless `async` is set. */
s32 __LongBlkMsgInt(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg, s32 async); /* untyped: caller-owned payload */

/* 0x804E52A0 - runs an interrupt or short bulk transfer (`ioctl` selects which); blocks unless `async` is set. */
s32 __IntrBlkMsgInt(s32 fd, s32 endpoint, s32 length, void* data, s32 ioctl, USBCallback callback, void* callbackArg, s32 async); /* untyped: caller-owned payload */

/* 0x804E5600 / 0x804E5680 / 0x804E5720 - asynchronous interrupt read, bulk read and bulk write. */
s32 IUSB_ReadIntrMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IUSB_ReadBlkMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IUSB_WriteBlkMsgAsync(s32 fd, s32 endpoint, u32 length, void* data, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

/* 0x804E57C0 - runs a control transfer with an optional data stage; blocks unless `async` is set. */
s32 __CtrlMsgInt(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg, u8 async); /* untyped: caller-owned payload */

/* 0x804E5D70 / 0x804E5E00 - asynchronous control transfers that read into or write from `data`. */
s32 IUSB_ReadCtrlMsgAsync(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IUSB_WriteCtrlMsgAsync(s32 fd, s32 requestType, s32 request, s32 value, s32 index, s32 length, void* data, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

/* 0x804E6000 / 0x804E63E0 - the isochronous transfer and the insertion notification registration. */
s32 IUSB_IsoMsgAsync(s32 fd, s32 endpoint, USBIsoRequest* iso, USBCallbackEx callback, void* callbackArg); /* untyped: caller-owned payload */
s32 IUSB_RegisterInsertionNotifyAsync(const char* path, s16 classId, s16 subId, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

/* 0x804E5E90 / 0x804E5EB0 - ioctl 0x1D without payload, and the device removal notification. */
s32 IUSB_Ioctl1D(s32 fd);
s32 IUSB_DeviceRemovalNotifyAsync(s32 fd, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

/* 0x804E4DB0 / 0x804E4E10 - closes the device descriptor `fd`. */
s32 IUSB_CloseDevice(s32 fd);
s32 IUSB_CloseDeviceAsync(s32 fd, USBCallback callback, void* callbackArg); /* untyped: caller-owned payload */

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_H */
