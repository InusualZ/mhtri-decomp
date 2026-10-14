/* BTE/bta_dm_cfg.h - the device manager configuration types and tables `BTE/bta_dm_cfg.c` owns (docs/plan.md 6.5
 *   rule 2). */
#ifndef MHTRI_BTE_BTA_DM_CFG_H
#define MHTRI_BTE_BTA_DM_CFG_H

#include "types.h"
#include "BTE/gki_time.h"

#define BTA_DM_PM_NUM_EVTS 9

/* size: 0x0C - the device manager's configuration: the class of device, the default link policy, the page timeout
 *   and the link supervision timeout. */
typedef struct tBTA_DM_CFG {
    /* +0x00 */ u8 dev_class[3];
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u16 policy_settings;
    /* +0x06 */ u16 page_timeout;
    /* +0x08 */ u16 link_timeout;
    /* +0x0A */ u8 pad_0x0A[2];
} tBTA_DM_CFG; /* size: 0x0C */

/* size: 0x03 - a resource manager or compression row: the BTA subsystem, its application and the setting. */
typedef struct tBTA_DM_RM {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 app_id;
    /* +0x02 */ u8 cfg;
} tBTA_DM_RM; /* size: 0x03 */

/* size: 0x03 - a power manager row: the BTA subsystem, its application and the index of its spec. */
typedef struct tBTA_DM_PM_CFG {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 app_id;
    /* +0x02 */ u8 spec_idx;
} tBTA_DM_PM_CFG; /* size: 0x03 */

/* size: 0x04 - one power manager action: the mode to request and after how long. */
typedef struct tBTA_DM_PM_ACTN {
    /* +0x00 */ u8 power_mode;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 timeout;
} tBTA_DM_PM_ACTN; /* size: 0x04 */

/* size: 0x4A - a power manager spec: the modes it allows and two actions per connection event. */
typedef struct tBTA_DM_PM_SPEC {
    /* +0x00 */ u8 allow_mask;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ tBTA_DM_PM_ACTN actn_tbl[BTA_DM_PM_NUM_EVTS][2];
} tBTA_DM_PM_SPEC; /* size: 0x4A */

#ifdef __cplusplus
extern "C" {
#endif

/* .rodata 0x80573548 - the device manager's configuration. */
extern const tBTA_DM_CFG bta_dm_cfg;

/* .sdata 0x80793D38 - 0x80793D48 - the resource manager, compression and power manager tables, the power manager
 * spec and the power modes, as the readers load them. */
extern tBTA_DM_RM* p_bta_dm_rm_cfg;
extern tBTA_DM_RM* p_bta_dm_compress_cfg;
extern tBTA_DM_PM_CFG* p_bta_dm_pm_cfg;
extern tBTA_DM_PM_SPEC* p_bta_dm_pm_spec;
extern BtmPmPwrMode* p_bta_dm_pm_md;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTA_DM_CFG_H */
