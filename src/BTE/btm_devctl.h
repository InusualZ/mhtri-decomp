/* BTE/btm_devctl.h - the device control constants `BTE/btm_devctl.c` keeps in btm_cb and the device control entry
 *   points it owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_DEVCTL_H
#define MHTRI_BTE_BTM_DEVCTL_H

#include "types.h"
#include "BTE/gki_time.h"

/* The longest local device name, without its terminator (the BTE source's BTM_MAX_LOC_BD_NAME_LEN). */
#define BTM_MAX_LOC_BD_NAME_LEN 31

/* The device states of the reset sequence (btm_cb.state). */
#define BTM_DEV_STATE_WAIT_RESET_CMPLT 0
#define BTM_DEV_STATE_WAIT_AFTER_RESET 1
#define BTM_DEV_STATE_1ST_HCI_CMD_SENT 2
#define BTM_DEV_STATE_2ND_HCI_CMD_SENT 3
#define BTM_DEV_STATE_3RD_HCI_CMD_SENT 4
#define BTM_DEV_STATE_READY 5

/* The device status a status callback reports. */
#define BTM_DEV_STATUS_UP 0
#define BTM_DEV_STATUS_DOWN 1

/* The `param` of the device control timers: the reset sequence, the local name read and the link policy read. */
#define TT_DEV_RESET 1
#define TT_DEV_RLN 2
#define TT_DEV_RLNKP 4

/* The longest vendor specific command result. */
#define BTM_MAX_VENDOR_SPECIFIC_LEN 255

/* size: 0x104 - the result of a vendor specific command: its opcode and the returned parameter bytes. */
typedef struct tBTM_VSC_CMPL {
    /* +0x000 */ u16 opcode;
    /* +0x002 */ u16 param_len;
    /* +0x004 */ u8 param_buf[BTM_MAX_VENDOR_SPECIFIC_LEN];
    /* +0x103 */ u8 pad_0x103;
} tBTM_VSC_CMPL; /* size: 0x104 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80482EA0 / 0x80482F40 - set the device control defaults and reset the controller, and report every pending
 * device request as reset. */
void btm_dev_init(void);
void btm_db_reset(void);

/* 0x8048308C - resets the controller like BTM_DeviceReset, with `p_cb` reporting the reset itself. */
void BTM_SendHciReset(BtmCompleteCallback p_cb);

/* 0x80483134 - whether the controller finished its reset (the device state is ready). */
u8 BTM_IsDeviceUp(void);

/* 0x80483240 - the device control timer expiry: retries the reset sequence or fails the local name read. */
void btm_dev_timeout(TIMER_LIST_ENT* p_tle);

/* 0x804834D8 / 0x8048369C / 0x80483764 / 0x80483820 - the HCI completions of the reset sequence (`p` is the event's
 * parameter stream). */
void btm_reset_complete(void);
void btm_read_hci_buf_size_complete(u8* p, u16 evt_len);
void btm_read_local_version_complete(u8* p, u16 evt_len);
void btm_read_local_features_complete(u8* p, u16 evt_len);

/* 0x80483D1C - stores the local device name `p_name` and sends it to the controller. */
u8 BTM_SetLocalDeviceName(char* p_name);

/* 0x80483DD4 / 0x80483EA0 - the HCI completions of the local name and local address reads. */
void btm_read_local_name_complete(u8* p, u16 evt_len);
void btm_read_local_addr_complete(u8* p, u16 evt_len);

/* The callback BTM_ReadLocalDeviceAddr reports the local address to. */
typedef void (*tBTM_LOCAL_ADDR_CB)(u8* bd_addr);

/* 0x80483E60 - hands the local address to `p_cback`. */
u8 BTM_ReadLocalDeviceAddr(tBTM_LOCAL_ADDR_CB p_cback);

/* 0x80483FB4 / 0x80483FC4 - the local device class and the controller's local feature bytes. */
u8* BTM_ReadDeviceClass(void);
u8* BTM_ReadLocalFeatures(void);

/* 0x804840E8 - reports the result of the vendor specific command `opcode` (`p`, `evt_len` bytes) to its callback. */
void btm_vsc_complete(u8* p, u16 opcode, u16 evt_len);

/* 0x8048418C - hands a vendor specific event (`p`, `evt_len` bytes) to the registered callback. */
void btm_vendor_specific_evt(u8* p, u8 evt_len);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DEVCTL_H */
