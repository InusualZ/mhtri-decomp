/* BTE/hidh_api.h - the HID host control block and the HID host API `BTE/hidh_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HIDH_API_H
#define MHTRI_BTE_HIDH_API_H

#include "BTE/hid_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8073C358 - the HID host control block (GUESS name hh_cb, the BTE source's). */
extern tHID_HOST_CB hh_cb;

/* 0x804900D4 - starts the HID record query of the device at `addr` into `p_db`; returns a HID result. */
u8 HID_HostGetSDPRecord(u8* addr, tSDP_DISCOVERY_DB* p_db, u32 db_len, tHID_HOST_SDP_CALLBACK* sdp_cback);

/* 0x8049019C - reads the HID record out of the finished query and reports it to the query's callback. */
void hidh_search_callback(u16 sdp_result);

/* 0x804905EC / 0x80490630 / 0x804906AC - clear the control block, register the device callback with L2CAP, and
 * remove every device and deregister; the last two return a HID result. */
void HID_HostInit(void);
u8 HID_HostRegister(tHID_HOST_DEV_CALLBACK* dev_cback);
u8 HID_HostDeregister(void);

/* 0x80490790 - 0x80490B8C - add, remove, open, write to and close a device; each returns a HID result. */
u8 HID_HostAddDev(u8* addr, u16 attr_mask, u8* handle);
u8 HID_HostRemoveDev(u8 dev_handle);
u8 HID_HostOpenDev(u8 dev_handle);
u8 HID_HostWriteDev(u8 dev_handle, u8 t_type, u8 param, u16 data, u8 report_id, BT_HDR* pbuf);
u8 HID_HostCloseDev(u8 dev_handle);

/* 0x80490C2C - registers the security level `sec_lvl` of the HID channels under `serv_name`; returns a HID result. */
u8 HID_HostSetSecurityLevel(char* serv_name, u8 sec_lvl);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HIDH_API_H */
