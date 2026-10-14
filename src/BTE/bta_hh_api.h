/* BTE/bta_hh_api.h - the BTA HID host API entry points `BTE/bta_hh_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_API_H
#define MHTRI_BTE_BTA_HH_API_H

#include "types.h"
#include "BTE/gki_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047FDC0 - registers the HID host with the BTA system and posts the enable request with `p_cback`. */
void BTA_HhEnable(u8 sec_mask, BtaHhCallback p_cback);

/* 0x8047FE48 - posts the disable request. */
void BTA_HhDisable(void);

/* 0x8047FE80 - posts the close request for the device `dev_handle`. */
void BTA_HhClose(u8 dev_handle);

/* 0x8047FEE4 - posts the open request for the device at `dev_bda`. */
void BTA_HhOpen(const u8* dev_bda, u8 mode, u8 sec_mask);

/* 0x8047FF98 - posts an output report for the device `dev_handle`. */
void BTA_HhSendData(u8 dev_handle, BT_HDR* p_data);

/* 0x80480028 / 0x804800D0 - post the request that adds a device with its descriptor, or removes one. */
void BTA_HhAddDev(const u8* bda, u16 attr_mask, u8 sub_class, u8 app_id, BtaHhDscpInfo dscp_info);
void BTA_HhRemoveDev(u8 dev_handle);

/* 0x8048013C - posts the HID host request that reports the ACL queue information. */
void BTA_HhGetAclQueueInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_API_H */
