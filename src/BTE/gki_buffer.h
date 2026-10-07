/* BTE/gki_buffer.h - the Bluetooth host stack entry points `BTE/gki_buffer.cpp` owns that the Wii remote driver calls
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GKI_BUFFER_H
#define MHTRI_BTE_GKI_BUFFER_H

#include "types.h"

/* size: 0x08 - the header of a Bluetooth stack buffer; `offset` bytes of room follow it before the payload. */
typedef struct BT_HDR {
    /* +0x00 */ u16 event;
    /* +0x02 */ u16 len;
    /* +0x04 */ u16 offset;
    /* +0x06 */ u16 layer_specific;
} BT_HDR; /* size: 0x08 */

/* size: 0x09 - a stack buffer with the first payload byte; the payload starts `offset` bytes into `data`. */
typedef struct BT_BUF {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 data[1];
} BT_BUF; /* size: 0x09 */

/* size: 0x0A - the sniff-mode parameters `BTM_SetPowerMode` takes. */
typedef struct BtmPmPwrMode {
    /* +0x00 */ u16 maxInterval;
    /* +0x02 */ u16 minInterval;
    /* +0x04 */ u16 attempt;
    /* +0x06 */ u16 timeout;
    /* +0x08 */ u8 mode;
    /* +0x09 */ u8 pad_0x09;
} BtmPmPwrMode; /* size: 0x0A */

/* size: 0x08 - the HID report descriptor `BTA_HhAddDev` registers with a device. */
typedef struct BtaHhDscpInfo {
    /* +0x00 */ u16 length;
    /* +0x04 */ const u8* descriptor;
} BtaHhDscpInfo; /* size: 0x08 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804777E0 - takes a buffer of at least `size` bytes from the stack's pools. */
BT_HDR* GKI_getbuf(u8 size);

/* 0x8047FF98 - sends the output report in `buf` to the HID device `handle`. */
void BTA_HhSendData(u8 handle, BT_HDR* buf);

/* 0x8047D98C / 0x8047DA7C - adds or removes the device at `bdAddr` in the stack's device database. */
u8 BTA_DmAddDevice(const u8* bdAddr, const u8* linkKey, u32 trustedMask, u8 trusted);
u8 BTA_DmRemoveDevice(const u8* bdAddr);

/* 0x80480028 / 0x804800D0 - registers or forgets a HID device (`dscp` is the report descriptor). */
void BTA_HhAddDev(const u8* bdAddr, u16 attrMask, u8 subClass, u8 appId, BtaHhDscpInfo dscp);
void BTA_HhRemoveDev(u8 handle);

/* 0x8047FEE4 - opens the HID connection to the device at `bdAddr` (`mode` and `security` are the stack's). */
void BTA_HhOpen(const u8* bdAddr, u8 mode, u8 security);

/* 0x8047A630 / 0x8047B128 / 0x80493A40 / 0x804A0290 - initialise the application layer and set the trace levels of the layers. */
void BTA_Init(void);
void bta_sys_set_trace_level(u8 level);
void L2CA_SetTraceLevel(u8 level);
void SDP_SetTraceLevel(u8 level);

/* 0x8047D788 - sets the host's device name (a NUL-terminated string). */
void BTA_DmSetDeviceName(const char* name);

/* 0x80482FF0 / 0x80483FEC - resets the controller, or sends the vendor specific HCI command `opcode` with `length` parameter bytes;
 * `callback` runs when the controller answered. */
void BTM_DeviceReset(void (*callback)(void));
void BTM_VendorSpecificCommand(u16 opcode, u8 length, const u8* params, void (*callback)(s32 result));

/* 0x80483F34 / 0x80484224 / 0x804814E8 / 0x804818B4 - class of device, page timeout and the default link policy and supervision timeout. */
void BTM_SetDeviceClass(const u8* deviceClass);
void BTM_WritePageTimeout(u16 timeout);
void BTM_SetDefaultLinkPolicy(u16 policy);
void BTM_SetDefaultLinkSuperTout(u16 timeout);

/* 0x80484154 / 0x80483FD4 / 0x80486450 - register the vendor event, device status and power mode callbacks. */
typedef void (*BtmVsEventCallback)(u8 length, u8* data);
typedef void (*BtmPmCallback)(const u8* bdAddr, u32 status, u16 value, u8 hciStatus);
void BTM_RegisterForVSEvents(BtmVsEventCallback callback);
void BTM_RegisterForDeviceStatusNotif(void (*callback)(u32 event));
u8 BTM_PmRegister(u8 mask, u8* pmId, BtmPmCallback callback);

/* 0x8047FE80 - closes the HID connection of `handle`. */
void BTA_HhClose(u8 handle);

/* 0x8047D7EC - sets whether the host is discoverable and connectable. */
void BTA_DmSetVisibility(u8 discoverable, u8 connectable);

/* 0x8047D8C8 - cancels a running device search. */
void BTA_DmSearchCancel(void);

/* 0x80483150 - restricts the frequency hopping to the channels from `first` to `last`; returns the stack's status. */
u8 BTM_SetAfhChannels(u8 first, u8 last);

/* 0x8048650C - requests the power mode `mode` for the device at `bdAddr`. */
u8 BTM_SetPowerMode(u8 pmId, const u8* bdAddr, BtmPmPwrMode* mode);

/* 0x804824F8 - drops the ACL link to the device at `bdAddr`. */
void btm_remove_acl(u8* bdAddr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GKI_BUFFER_H */
