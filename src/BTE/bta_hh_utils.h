/* BTE/bta_hh_utils.h - the BTA HID host utilities `BTE/bta_hh_utils.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_UTILS_H
#define MHTRI_BTE_BTA_HH_UTILS_H

#include "BTE/bta_hh_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804806E4 - returns the index of the device control block for `bda`, taking a free one when it is unknown. */
u8 bta_hh_find_cb(u8* bda);

/* 0x80480820 - resets a device control block, keeping its index. */
void bta_hh_clean_up_kdev(tBTA_HH_DEV_CB* p_cb);

/* 0x804808A0 - records a registered device's handle, attributes and report descriptor in its control block. */
void bta_hh_add_device_to_list(tBTA_HH_DEV_CB* p_cb, u8 handle, u16 attr_mask, tBTA_HH_DEV_DSCP_INFO* p_dscp_info,
                               u8 sub_class, u8 app_id);

/* 0x8048095C - returns whether the device subclass is a supported type, storing its application id. */
u8 bta_hh_tod_spt(tBTA_HH_DEV_CB* p_cb, u8 sub_class);

/* Dead-stripped in retail - parse a boot-mode keyboard or mouse report. */
void bta_hh_parse_keybd_rpt(u8* p_data, u8* p_report, u16 report_len);
void bta_hh_parse_mice_rpt(u8* p_data, u8* p_report, u16 report_len);

/* 0x804809B0 - prints the device table. */
void bta_hh_trace_dev_db(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_UTILS_H */
