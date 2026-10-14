/*
 * BTE/bta_hh_cfg.c - the HID host configuration of the Broadcom BTA layer (data only, GUESS file name): the device
 *   table and the config that points at it.
 *
 * RANGE. no .text; .rodata 0x805735D8-0x805735E8; .sdata 0x80793D50-0x80793D60. The .sdata pointer 0x80793D58
 *   (to the config 0x805735D8, which points at the device table 0x80793D50) is read by bta_hh_start_sdp
 *   (BTE/bta_hh_act.c) and bta_hh_tod_spt (BTE/bta_hh_utils.c) only. It follows BTE/bta_dm_cfg.c and precedes
 *   BTE/bta_sys_cfg.c in .rodata and .sdata.
 * FLAGS. no code; the band's Wii/1.3 `cflags_base` (the head's) places the tables the same way.
 * NAMES. the BTE source's (bta_hh_cfg.c), GUESS: 0x80793D50 bta_hh_spt_tod, 0x805735D8 bta_hh_cfg, 0x80793D58
 *   p_bta_hh_cfg. The file name is a GUESS from Broadcom's bta_hh_cfg.c.
 * RESIDUALS. the layouts are read from bta_hh_tod_spt (the count at +0, the table at +4, 2-byte rows of device type
 *   and application id); the SDP database size at +8 is the BTE source's field, no reader in the band.
 * SHAPES. plain C; the device table is not const (it sits in .sdata).
 */

#include "types.h"

/* size: 0x02 - a supported HID device type and the application that serves it. */
typedef struct tBTA_HH_SPT_TOD {
    /* +0x00 */ u8 tod;
    /* +0x01 */ u8 app_id;
} tBTA_HH_SPT_TOD; /* size: 0x02 */

/* size: 0x0C - the HID host configuration: the supported device types and the SDP database size. */
typedef struct tBTA_HH_CFG {
    /* +0x00 */ u8 max_devt_spt;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ tBTA_HH_SPT_TOD* p_devt_list;
    /* +0x08 */ u16 sdp_db_size;
    /* +0x0A */ u8 pad_0x0A[2];
} tBTA_HH_CFG; /* size: 0x0C */

/* .sdata 0x80793D50 - the supported device types. */
tBTA_HH_SPT_TOD bta_hh_spt_tod[] = {
    {0x20, 0x01},
    {0x10, 0x02},
    {0x01, 0x03},
    {0x31, 0x02},
};

/* .rodata 0x805735D8 - the HID host configuration. */
const tBTA_HH_CFG bta_hh_cfg = {4, {0}, bta_hh_spt_tod, 0x0400};

/* .sdata 0x80793D58 - the configuration as its readers load it. */
tBTA_HH_CFG* p_bta_hh_cfg = (tBTA_HH_CFG*)&bta_hh_cfg;
