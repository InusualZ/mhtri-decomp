/*
 * BTE/bta_dm_cfg.c - the device manager configuration of the Broadcom BTA layer (data only, GUESS file name): the
 *   device config, the resource manager and compression tables and the power manager tables.
 *
 * RANGE. no .text; .rodata 0x80573548-0x805735D8; .sdata 0x80793D38-0x80793D50; .sbss2 0x8079D7E0-0x8079D7E8.
 *   The device config 0x80573548 is read by bta_dm_enable (BTE/bta_dm_act.c) and 0x8047DCDC (BTE/bta_dm_pm.c);
 *   the .sdata pointers 0x80793D38 (to 0x80573554, read by bta_dm_rm_cback), 0x80793D3C (to 0x80573560, read by
 *   bta_dm_compress_cback), 0x80793D40 (to the zero table 0x8079D7E0), 0x80793D44 (to 0x80573574) and 0x80793D48
 *   (to 0x805735C0) are read by BTE/bta_dm_act.c and BTE/bta_dm_pm.c only. Data-only files sit between
 *   BTE/uusb_ppc.c and BTE/bte_hcisu.c in every section they occupy (.rodata after BTE/hcisu_h2.c's interface
 *   table, .sdata between 0x80793D38 and BTE/bte_logmsg.c's 0x80793D68), in the dm / hh / sys order of their
 *   readers.
 * FLAGS. no code; the band's Wii/1.3 `cflags_base` (the head's) places the tables the same way.
 * NAMES. the BTE source's (bta_dm_cfg.c), GUESS: 0x80573548 bta_dm_cfg, 0x80573554 bta_dm_rm_cfg, 0x80573560
 *   bta_dm_compress_cfg, 0x8079D7E0 bta_dm_pm_cfg, 0x80573574 bta_dm_pm_spec, 0x805735C0 bta_dm_pm_md and the
 *   pointers 0x80793D38..0x80793D48 p_bta_dm_rm_cfg, p_bta_dm_compress_cfg, p_bta_dm_pm_cfg, p_bta_dm_pm_spec,
 *   p_bta_dm_pm_md. The file name is a GUESS from Broadcom's bta_dm_cfg.c.
 * RESIDUALS. the row layouts are read from the readers: the 3-byte resource manager and compression rows (id,
 *   application id, setting; row 0 carries the row count in its second byte), the 74-byte power manager spec
 *   (stride `mulli 74` in BTE/bta_dm_pm.c) and the 10-byte power mode (BTE/gki_time.h's BtmPmPwrMode, BTM_SetPowerMode
 *   is passed `p_bta_dm_pm_md + 10`). The field names past those offsets are the BTE source's.
 * SHAPES. plain C, const tables and the non-const pointers to them the readers load; the all-zero power manager
 *   config is a const that MWCC places in .sbss2.
 */

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

/* .rodata 0x80573548 - the device manager's configuration. */
const tBTA_DM_CFG bta_dm_cfg = {{0x40, 0x02, 0x04}, 0, 0x000F, 0x2000, 0x1388};

/* .rodata 0x80573554 - the resource manager rows; row 0 holds the row count. */
const tBTA_DM_RM bta_dm_rm_cfg[] = {
    {0x00, 0x02, 0x01},
    {0x0E, 0x01, 0x02},
    {0x0E, 0x02, 0x02},
    {0x12, 0xFF, 0x01},
};

/* .rodata 0x80573560 - the compression rows; row 0 holds the row count. */
const tBTA_DM_RM bta_dm_compress_cfg[] = {
    {0x00, 0x05, 0x00},
    {0x0A, 0xFF, 0x01},
    {0x08, 0xFF, 0x01},
    {0x06, 0xFF, 0x01},
    {0x07, 0xFF, 0x01},
    {0x12, 0xFF, 0x02},
};

/* .sbss2 0x8079D7E0 - the power manager rows (none configured). */
const tBTA_DM_PM_CFG bta_dm_pm_cfg[2] = {0};

/* .rodata 0x80573574 - the power manager spec. */
const tBTA_DM_PM_SPEC bta_dm_pm_spec[] = {
    {0x03,
     0,
     {
         {{0x02, 0, 0x1388}, {0x00, 0, 0x0000}},
         {{0x10, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x00, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x00, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x00, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x02, 0, 0x1388}, {0x00, 0, 0x0000}},
         {{0x00, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x00, 0, 0x0000}, {0x00, 0, 0x0000}},
         {{0x08, 0, 0x1388}, {0x00, 0, 0x0000}},
     }},
};

/* .rodata 0x805735C0 - the power modes the specs request: sniff, then park. */
const BtmPmPwrMode bta_dm_pm_md[] = {
    {0x0190, 0x00C8, 0x0004, 0x0004, 0x02, 0},
    {0x0320, 0x0190, 0x0000, 0x0000, 0x03, 0},
};

/* .sdata 0x80793D38..0x80793D48 - the tables as the readers load them. */
tBTA_DM_RM* p_bta_dm_rm_cfg = (tBTA_DM_RM*)bta_dm_rm_cfg;
tBTA_DM_RM* p_bta_dm_compress_cfg = (tBTA_DM_RM*)bta_dm_compress_cfg;
tBTA_DM_PM_CFG* p_bta_dm_pm_cfg = (tBTA_DM_PM_CFG*)bta_dm_pm_cfg;
tBTA_DM_PM_SPEC* p_bta_dm_pm_spec = (tBTA_DM_PM_SPEC*)bta_dm_pm_spec;
BtmPmPwrMode* p_bta_dm_pm_md = (BtmPmPwrMode*)bta_dm_pm_md;
