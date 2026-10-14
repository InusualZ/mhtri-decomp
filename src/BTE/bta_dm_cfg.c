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
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)); no code, so no alignment evidence.
 * NAMES. the map's; the file name is a GUESS from Broadcom's bta_dm_cfg.c.
 * RESIDUALS. the data is unclaimed by source.
 * SHAPES. plain C, const tables and pointers to them.
 */
