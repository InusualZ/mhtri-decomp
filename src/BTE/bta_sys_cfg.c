/*
 * BTE/bta_sys_cfg.c - the system configuration of the Broadcom BTA layer (data only, GUESS file name).
 *
 * RANGE. no .text; .sdata 0x80793D60-0x80793D68; .sdata2 0x8079D090-0x8079D098. The .sdata pointer 0x80793D60 (to
 *   the config 0x8079D090) is read by bta_sys_init and bta_sys_sendmsg (BTE/bta_sys_main.c) only. It follows
 *   BTE/bta_hh_cfg.c in .sdata and BTE/hcisu_h2.c's tables in .sdata2.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)); no code, so no alignment evidence.
 * NAMES. the map's; the file name is a GUESS from Broadcom's bta_sys_cfg.c.
 * RESIDUALS. the data is unclaimed by source.
 * SHAPES. plain C, a const config and a pointer to it.
 */
