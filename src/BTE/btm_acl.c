/*
 * BTE/btm_acl.c - the BTM ACL link management: the ACL table, link creation and removal, role switch, link policy,
 *   remote version/features, RSSI and link quality reads and the packet-type changes.
 *
 * RANGE. .text 0x80480A90-0x804827D4 (29 functions, 0x1D44 B); .data 0x80613B60-0x80613F80. Medium-strong start: the
 *   btm_* family opens with btm_acl_init after bta_hh_utils.c's last function, and BTE/gki_time.c's .data ends with
 *   the bta_hh tables exactly at 0x80613B60, the first object 0x80480B30 reads. It ends with
 *   btm_chg_all_acl_pkt_types, the last function of the BTE source's btm_acl.c; btm_dev.c's BTM_SecAddDevice follows.
 *   Every .data object is read only by this range's functions; the .data end 0x80613F80 is 8-aligned with no pad, so
 *   that seam is where the readers change, not a pad.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x80480B30 btm_acl_created (between btm_handle_to_acl_index and
 *   btm_acl_removed in the BTE source order; fills an ACL entry), 0x804819F8 btm_get_acl_disc_reason_code (returns
 *   one btm_cb byte; between BTM_GetNumAclLinks and BTM_GetHCIConnHandle as in the BTE source).
 * RESIDUALS. every body is unwritten: 0x80480A90-0x804827D4 (29 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
