/* BTE/bta_hh_main.h - the BTA HID host state machine entry points and control block `BTE/bta_hh_main.c` owns
 *   (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_MAIN_H
#define MHTRI_BTE_BTA_HH_MAIN_H

#include "BTE/bta_hh_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x80739468 - the HID host control block. */
extern tBTA_HH_CB bta_hh_cb;

/* 0x80480190 - runs the event `event` through the state machine of the device `p_cb`. */
void bta_hh_sm_execute(tBTA_HH_DEV_CB* p_cb, u16 event, tBTA_HH_DATA* p_data);

/* 0x804804E4 - the BTA system event handler: dispatches a HID host message; returns TRUE. */
u8 bta_hh_hdl_event(BT_HDR* p_msg);

/* 0x80480630 - returns the trace name of an internal event. */
char* bta_hh_evt_code(u16 evt_code);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_MAIN_H */
