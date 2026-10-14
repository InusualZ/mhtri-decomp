/* WUD/WUDHidHost.h - the HID host event callback and the BTE callouts `WUD/WUDHidHost.cpp` owns (docs/plan.md 6.5 rule 2). */
#ifndef WUD_WUDHIDHOST_H
#define WUD_WUDHIDHOST_H

#include "types.h"
#include "BTE/gki_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80500130 - the HID host event callback `__wudSecurityEventStackCallback` installs with `BTA_HhEnable`. */
void WUDiHidHostEventCallback(u8 event, union BtaHhEventData* data);
/* 0x805006B0 - passes a HID data report to the receive callback when the application id is the Wii remote's. */
void bta_hh_co_data(u8 handle, u8* report, u16 length, u8 mode, u8 subClass, u8 appId);
/* 0x805006F0 / 0x80500700 - log a HID host open and close. */
void bta_hh_co_open(void);
void bta_hh_co_close(void);
/* 0x80500710 - the BTE compression memory callout; returns zero (no memory). */
s32 bta_dm_co_get_compress_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* WUD_WUDHIDHOST_H */
