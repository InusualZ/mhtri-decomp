/*
 * BTE/bta_hh_main.c - the BTA HID host state machine: bta_hh_sm_execute, the event handler and the event-code
 *   trace names, with the state tables.
 *
 * RANGE. .text 0x80480190-0x804806E4 (3 functions, 0x554 B); .rodata 0x805737B8-0x80573840; .data
 *   0x806135C0-0x806138A0; .bss 0x80739468-0x80739698. Strong start: bta_hh_sm_execute is the only reader of the
 *   .rodata state tables and, with bta_hh_hdl_event and bta_hh_evt_code, of the .data from 0x806135C0. Medium end:
 *   bta_hh_find_cb opens the utility run and its .data 0x806138A0. The .bss control block 0x80739468 (0x230 B) is read
 *   by every bta_hh file but the API; it is GUESS Broadcom's bta_hh_cb, a bta_hh_main.c global, and BTE/bta_hh_act.c
 *   would give the same layout.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80480190-0x804806E4 (3 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
