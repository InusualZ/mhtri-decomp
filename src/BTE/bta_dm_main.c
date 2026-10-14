/*
 * BTE/bta_dm_main.c - the BTA device manager state machines: bta_dm_sm_execute and bta_dm_search_sm_execute with
 *   their action and state tables.
 *
 * RANGE. .text 0x8047DAFC-0x8047DC5C (2 functions, 0x160 B); .rodata 0x80573690-0x805737B8; .bss
 *   0x807392B8-0x80739438. Medium: the two state-machine drivers between the API run and bta_dm_init_pm. The .rodata
 *   0x80573690-0x805737B8 is read only by them (the action table, the search state tables 0x80573738-0x805737A8 and
 *   their index 0x805737A8). The .bss is GUESS: 0x807392B8 (0x7C B, read by the search actions and
 *   bta_dm_search_sm_execute) and 0x80739334 (0x104 B, read by the actions, the API and the power manager) are
 *   Broadcom's bta_dm_search_cb and bta_dm_cb, which bta_dm_main.c defines; the readers alone allow them in
 *   BTE/bta_dm_act.c too, with the same layout.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8047DAFC-0x8047DC5C (2 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
