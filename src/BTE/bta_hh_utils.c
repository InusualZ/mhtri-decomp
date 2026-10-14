/*
 * BTE/bta_hh_utils.c - the BTA HID host utilities: control-block lookup, device clean-up, the device list and the
 *   trace helpers.
 *
 * RANGE. .text 0x804806E4-0x80480A90 (5 functions, 0x3AC B); .data 0x806138A0-0x80613B60; .sbss2
 *   0x8079D7E8-0x8079D7F0. Medium start: bta_hh_find_cb opens the bta_hh_* utility names after bta_hh_evt_code; the
 *   .data 0x806138A0 is read only by bta_hh_find_cb and bta_hh_trace_dev_db, and 0x80613924 only by
 *   bta_hh_add_device_to_list. Strong end: btm_acl_init opens BTE/btm_acl.c, whose .data starts at 0x80613B60. The
 *   .sbss2 all-zero address 0x8079D7E8 is read only by bta_hh_find_cb (BTE/bd.c's header has the alternative owner).
 *   The cfg pointer 0x80793D58 that bta_hh_tod_spt reads stays in BTE/gki_time.c.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804806E4-0x80480A90 (5 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
