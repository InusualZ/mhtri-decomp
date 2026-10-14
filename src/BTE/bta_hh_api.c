/*
 * BTE/bta_hh_api.c - the BTA HID host API: enable and disable, open and close, send data, add and remove a device
 *   and the ACL queue information.
 *
 * RANGE. .text 0x8047FDC0-0x80480190 (8 functions, 0x3D0 B); .data 0x80613590-0x806135C0; .sdata2
 *   0x8079D0B0-0x8079D0B8. Strong: the BTA_Hh* run, every function posting a GKI message through bta_sys_sendmsg;
 *   bta_hh_sm_execute follows. The .data string 0x80613590 is read only by BTA_HhOpen, the .sdata2 double only by
 *   BTA_HhEnable.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed row (GUESS, not applied): 0x8047FE48 BTA_HhDisable (posts event 0x170D with no
 *   payload, between BTA_HhEnable and BTA_HhClose).
 * RESIDUALS. every body is unwritten: 0x8047FDC0-0x80480190 (8 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
