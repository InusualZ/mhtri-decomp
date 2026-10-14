/*
 * BTE/bta_sys_cfg.c - the system configuration of the Broadcom BTA layer (data only, GUESS file name).
 *
 * RANGE. no .text; .sdata 0x80793D60-0x80793D68; .sdata2 0x8079D090-0x8079D098. The .sdata pointer 0x80793D60 (to
 *   the config 0x8079D090) is read by bta_sys_init and bta_sys_sendmsg (BTE/bta_sys_main.c) only. It follows
 *   BTE/bta_hh_cfg.c in .sdata and BTE/hcisu_h2.c's tables in .sdata2.
 * FLAGS. no code; the band's Wii/1.3 `cflags_base` (the head's) places the tables the same way.
 * NAMES. the BTE source's (bta_sys_cfg.c), GUESS: 0x8079D090 bta_sys_cfg, 0x80793D60 p_bta_sys_cfg. The file name is
 *   a GUESS from Broadcom's bta_sys_cfg.c.
 * RESIDUALS. the layout is read from bta_sys_init (the timer at +3, the trace level at +4); the mailbox event and
 *   mailbox at +0 / +2 are the BTE source's fields (the values are the BTU task's mailbox 2 and its event mask).
 * SHAPES. plain C, a const config and a pointer to it.
 */

#include "types.h"

/* size: 0x06 - the BTA system configuration: the BTU task mailbox and timer BTA uses and its initial trace level. */
typedef struct tBTA_SYS_CFG {
    /* +0x00 */ u16 mbox_evt;
    /* +0x02 */ u8 mbox;
    /* +0x03 */ u8 timer;
    /* +0x04 */ u8 trace_level;
    /* +0x05 */ u8 pad_0x05;
} tBTA_SYS_CFG; /* size: 0x06 */

/* .sdata2 0x8079D090 - the BTA system configuration. */
const tBTA_SYS_CFG bta_sys_cfg = {0x0004, 2, 1, 0};

/* .sdata 0x80793D60 - the configuration as its readers load it. */
tBTA_SYS_CFG* p_bta_sys_cfg = (tBTA_SYS_CFG*)&bta_sys_cfg;
