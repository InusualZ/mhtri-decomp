/* BTE/btm_devctl.h - the device control entry points `BTE/btm_devctl.c` owns that other units call (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTM_DEVCTL_H
#define MHTRI_BTE_BTM_DEVCTL_H

#include "types.h"
#include "BTE/gki_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80483134 - whether the controller finished its reset (the device state is ready). */
u8 BTM_IsDeviceUp(void);

/* 0x80483FC4 - the controller's local feature bytes. */
u8* BTM_ReadLocalFeatures(void);

struct TIMER_LIST_ENT;
/* 0x80483240 - handles the expiry of the device control timer `p_tle`. */
void btm_dev_timeout(struct TIMER_LIST_ENT* p_tle);

/* 0x804834D8 */
void btm_reset_complete(void);
/* 0x8048369C */
void btm_read_hci_buf_size_complete(u8* p, u16 evt_len);
/* 0x80483764 */
void btm_read_local_version_complete(u8* p, u16 evt_len);
/* 0x80483820 */
void btm_read_local_features_complete(u8* p, u16 evt_len);
/* 0x80483DD4 */
void btm_read_local_name_complete(u8* p, u16 evt_len);
/* 0x80483EA0 */
void btm_read_local_addr_complete(u8* p, u16 evt_len);
/* 0x804840E8 */
void btm_vsc_complete(u8* p, u16 opcode, u16 evt_len);
/* 0x8048418C */
void btm_vendor_specific_evt(u8* p, u8 evt_len);

/* The callback BTM_ReadLocalDeviceAddr reports the local address to. */
typedef void (*tBTM_LOCAL_ADDR_CB)(u8* bd_addr);

/* 0x80483D1C - sets the local device name. */
u8 BTM_SetLocalDeviceName(char* p_name);

/* 0x80483E60 - reads the local address; `p_cback` gets it. */
u8 BTM_ReadLocalDeviceAddr(tBTM_LOCAL_ADDR_CB p_cback);

/* 0x8048308C - resets the controller; `p_cb` runs when it answered. */
void BTM_SendHciReset(BtmCompleteCallback p_cb);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_DEVCTL_H */
