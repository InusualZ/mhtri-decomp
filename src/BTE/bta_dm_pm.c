/*
 * BTE/bta_dm_pm.c - the BTA device manager power management: init, the connection-status callback, the power mode
 *   selection and the BTM power-mode and timer callbacks.
 *
 * RANGE. .text 0x8047DC5C-0x8047E678 (8 functions, 0xA1C B); .data 0x80613048-0x80613098; .bss 0x80739438-0x80739468.
 *   Strong start: the .data starts after an 8-byte pad (0x80613043 -> 0x80613048) and bta_dm_init_pm is the first
 *   bta_dm_pm.c function. Strong end: BTE/bta_hh_act.c's .data starts after an 8-byte pad (0x80613091 -> 0x80613098),
 *   and 0x8047E66C, which forwards to bta_dm_pm_set_mode, is the last function before bta_hh_api_enable. Both .data
 *   strings are read only by 0x8047DCDC and bta_dm_pm_set_mode. The .bss object 0x80739438 (0x30 B, read by
 *   bta_dm_enable, the compression callback and this range) is GUESS Broadcom's bta_dm_conn_srvcs, a bta_dm_pm.c
 *   global; BTE/bta_dm_main.c or BTE/bta_dm_act.c would give the same layout.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows: 0x8047DCC4, 0x8047DCDC (the connection-status callback the power manager
 *   registers) and 0x8047E66C (calls bta_dm_pm_set_mode with mode 1); names not pinned.
 * RESIDUALS. every body is unwritten: 0x8047DC5C-0x8047E678 (8 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
