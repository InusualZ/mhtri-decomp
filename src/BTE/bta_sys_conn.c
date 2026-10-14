/*
 * BTE/bta_sys_conn.c - the BTA system connection manager: the resource-manager, compression and power-manager callback
 *   registrations, the connection open and close notifications and three power-manager status forwards.
 *
 * RANGE. .text 0x8047AC98-0x8047AEEC (8 functions, 0x254 B); no data. Medium start: 0x8047AC98, 0x8047ACA8 and
 *   bta_sys_pm_register each store one callback into the bta_sys control block (+0x80, +0x88, +0x84), the registrations
 *   Broadcom's bta_sys_conn.c opens with, after the two address helpers of BTE/bd.c, which read no data. Medium end:
 *   bta_sys_init opens BTE/bta_sys_main.c, whose .data starts after an 8-byte pad (0x80612ECB -> 0x80612ED0). Every
 *   function reads the control block 0x807391F8, which BTE/bta_sys_main.c owns.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x8047AC98 bta_sys_rm_register, 0x8047ACA8
 *   bta_sys_compress_register (the store offsets beside bta_sys_pm_register's); 0x8047AE50, 0x8047AE84 and 0x8047AEB8
 *   call the power-manager callback with status 5, 6 and 7 (the BTA_SYS_* connection-status forwards, not pinned).
 * RESIDUALS. every body is unwritten: 0x8047AC98-0x8047AEEC (8 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
