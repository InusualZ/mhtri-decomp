/* BTE/bta_hh_cfg.h - the HID host configuration type and pointer `BTE/bta_hh_cfg.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTA_HH_CFG_H
#define MHTRI_BTE_BTA_HH_CFG_H

#include "types.h"

/* size: 0x02 - one supported device type: the class-of-device minor type and the application it opens. */
typedef struct tBTA_HH_SPT_TOD {
    /* +0x00 */ u8 tod;
    /* +0x01 */ u8 app_id;
} tBTA_HH_SPT_TOD; /* size: 0x02 */

/* size: 0x0C - the HID host configuration: the supported device types and the SDP database size. */
typedef struct tBTA_HH_CFG {
    /* +0x00 */ u8 max_devt_spt;
    /* +0x04 */ tBTA_HH_SPT_TOD* p_devt_list;
    /* +0x08 */ u16 sdp_db_size;
} tBTA_HH_CFG; /* size: 0x0C */

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata 0x80793D58 - the HID host configuration. */
extern tBTA_HH_CFG* p_bta_hh_cfg;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_HH_CFG_H */
