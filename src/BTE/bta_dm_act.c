/*
 * BTE/bta_dm_act.c - the BTA device manager actions: enable and disable, bonding and PIN/authorisation replies, the
 *   inquiry and remote-name search, the SDP service discovery, the BTM security and ACL callbacks, link quality,
 *   compression and the resource-manager callback.
 *
 * RANGE. .text 0x8047B354-0x8047D690 (55 functions, 0x233C B); .rodata 0x805735E8-0x80573690; .data
 *   0x80612F08-0x80613048; .bss 0x80739288-0x807392B8; .sdata2 0x8079D098-0x8079D0A0. Strong start: bta_dm_enable is
 *   the first reader of the bta_dm data and Broadcom's first bta_dm_act.c function. Strong end: BTA_EnableBluetooth
 *   opens the API run, and BTE/bta_dm_pm.c's .data starts after an 8-byte pad (0x80613043 -> 0x80613048). Every .data
 *   object (the trace strings 0x80612F08-0x80613043) is read only by this range. The .rodata table 0x80573618 is also
 *   read by BTA_DmAddDevice (the service-id lookup); 0x805735E8 and 0x80573678 are read only here. The .bss
 *   object 0x80739288 (0x30 B) is read only by bta_dm_enable and the two compression callbacks. The .sdata2 pair
 *   0x8079D098 is a local initialiser bta_dm_find_services copies to its stack.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8047B354-0x8047D690 (55 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
