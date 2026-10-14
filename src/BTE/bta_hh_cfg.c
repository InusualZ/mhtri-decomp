/*
 * BTE/bta_hh_cfg.c - the HID host configuration of the Broadcom BTA layer (data only, GUESS file name): the device
 *   table and the config that points at it.
 *
 * RANGE. no .text; .rodata 0x805735D8-0x805735E8; .sdata 0x80793D50-0x80793D60. The .sdata pointer 0x80793D58
 *   (to the config 0x805735D8, which points at the device table 0x80793D50) is read by bta_hh_start_sdp
 *   (BTE/bta_hh_act.c) and bta_hh_tod_spt (BTE/bta_hh_utils.c) only. It follows BTE/bta_dm_cfg.c and precedes
 *   BTE/bta_sys_cfg.c in .rodata and .sdata.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)); no code, so no alignment evidence.
 * NAMES. the map's; the file name is a GUESS from Broadcom's bta_hh_cfg.c.
 * RESIDUALS. the data is unclaimed by source.
 * SHAPES. plain C, a const config and the table it points at.
 */
