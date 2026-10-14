/* BTE/gki_time.h - the GKI timer list type `BTE/gki_time.c` owns and the BTA/BTM calls of the Wii remote driver; it
 *   includes the headers of the files cut from the BTE head (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GKI_TIME_H
#define MHTRI_BTE_GKI_TIME_H

#include "types.h"
#include "BTE/gki_buffer.h"
#include "BTE/l2c_api.h"
#include "BTE/sdp_api.h"
#include "BTE/gki_ppc.h"
#include "BTE/bte_hcisu.h"
#include "BTE/bte_logmsg.h"
#include "BTE/bta_init.h"
#include "BTE/btu_task1.h"

/* size: 0x0A - the sniff-mode parameters `BTM_SetPowerMode` takes. */
typedef struct BtmPmPwrMode {
    /* +0x00 */ u16 maxInterval;
    /* +0x02 */ u16 minInterval;
    /* +0x04 */ u16 attempt;
    /* +0x06 */ u16 timeout;
    /* +0x08 */ u8 mode;
    /* +0x09 */ u8 pad_0x09;
} BtmPmPwrMode; /* size: 0x0A */

/* size: 0x0A - the controller's version block `BTM_ReadLocalVersion` copies out (BTE `tBTM_VERSION_INFO`). */
typedef struct BtmVersionInfo {
    /* +0x00 */ u8 hciVersion;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 hciRevision;
    /* +0x04 */ u8 lmpVersion;
    /* +0x05 */ u8 pad_0x05;
    /* +0x06 */ u16 manufacturer;
    /* +0x08 */ u16 lmpSubversion;
} BtmVersionInfo; /* size: 0x0A */

/* size: 0x0A - the inquiry parameters `BTA_DmSearch` copies into its message (GUESS names, BTE order). */
typedef struct BtaDmInq {
    /* +0x00 */ u8 mode;
    /* +0x01 */ u8 duration;
    /* +0x02 */ u8 maxResponses;
    /* +0x03 */ u8 filterType;
    /* +0x04 */ u8 filterCond[6];
} BtaDmInq; /* size: 0x0A */

/* The event payloads of the HID host and the device manager security callbacks; WUD/WUDHidHost.cpp and WUD/wud.cpp define them. */
union BtaHhEventData;
union BtaDmSecEventData;

/* The HID host event callback `BTA_HhEnable` installs. */
typedef void (*BtaHhCallback)(u8 event, union BtaHhEventData* data);
/* The device manager security event callback `BTA_EnableBluetooth` installs. */
typedef void (*BtaDmSecCallback)(u8 event, union BtaDmSecEventData* data);
/* The device search event callback `BTA_DmSearch` installs. */
typedef void (*BtaDmSearchCallback)(s32 event, const u8* data);
/* The completion callback of a BTM command; the payload's type depends on the command. */
typedef void (*BtmCompleteCallback)(void* result); /* untyped: caller-owned payload, one type per command */

/* size: 0x08 - the HID report descriptor `BTA_HhAddDev` registers with a device. */
typedef struct BtaHhDscpInfo {
    /* +0x00 */ u16 length;
    /* +0x04 */ const u8* descriptor;
} BtaHhDscpInfo; /* size: 0x08 */

/* size: 0x18 - a BTU timer list entry `btu_start_timer` links into its list. */
typedef struct TIMER_LIST_ENT {
    /* +0x00 */ struct TIMER_LIST_ENT* p_next;
    /* +0x04 */ struct TIMER_LIST_ENT* p_prev;
    /* +0x08 */ u32 ticks;
    /* +0x0C */ u32 unused_0x0C;
    /* +0x10 */ u32 param;
    /* +0x14 */ u16 event;
    /* +0x16 */ u8 in_use;
    /* +0x17 */ u8 pad_0x17;
} TIMER_LIST_ENT; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

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

/* 0x8047B128 - sets the trace levels of the layers (the L2CAP and SDP layers' are BTE/l2c_api.h's and
 * BTE/sdp_api.h's). */
void bta_sys_set_trace_level(u8 level);

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

/* 0x8047D690 / 0x8047D718 - start the device manager with its security callback, or stop it; the start returns the status. */
u8 BTA_EnableBluetooth(BtaDmSecCallback callback);
void BTA_DisableBluetooth(void);

/* 0x8047D844 - starts a device search with the inquiry `inquiry` for the services `services`. */
void BTA_DmSearch(const BtaDmInq* inquiry, u32 services, BtaDmSearchCallback callback);

/* 0x8047D900 - answers a PIN request of the device at `bdAddr`; the PIN is copied only when `accept` is non-zero. */
void BTA_DmPinReply(const u8* bdAddr, u8 accept, u8 pinLength, const u8* pin);

/* 0x8047FDC0 - enables the HID host with the security mask `secMask` and its event callback. */
void BTA_HhEnable(u8 secMask, BtaHhCallback callback);

/* 0x80483EE8 - copies the controller's version block to `version`; returns the stack's status. */
u8 BTM_ReadLocalVersion(BtmVersionInfo* version);

/* 0x804842C4 / 0x8048439C / 0x8048446C - read, write or delete the link keys the controller stores (`bdAddr` NULL = every
 * key); `numKeys` addresses and keys are written; each returns the stack's status. */
u8 BTM_ReadStoredLinkKey(const u8* bdAddr, BtmCompleteCallback callback);
u8 BTM_WriteStoredLinkKey(u8 numKeys, const u8* bdAddrs, const u8* linkKeys, BtmCompleteCallback callback);
u8 BTM_DeleteStoredLinkKey(const u8* bdAddr, BtmCompleteCallback callback);

/* 0x804880E8 - registers the security level of a service on `psm`; returns whether it was stored. */
u8 BTM_SetSecurityLevel(u8 is_originator, char* p_name, u8 service_id, u8 sec_level, u16 psm, u32 mx_proto_id,
                        u32 mx_chan_id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GKI_TIME_H */
