/*
 * BTE/bta_hh_act.c - the BTA HID host actions: enable and disable, the SDP discovery, open and close, data,
 *   handshake and control, the ACL queue information, device writes and the HID host callback.
 *
 * RANGE. .text 0x8047E678-0x8047FDC0 (18 functions, 0x1748 B); .data 0x80613098-0x80613590; .sdata
 *   0x80793D80-0x80793D88. Strong start: the .data starts after an 8-byte pad (0x80613091 -> 0x80613098) and
 *   bta_hh_api_enable is the first reader of the bta_hh control block. Strong end: BTA_HhEnable opens the API run.
 *   Every .data object (strings, the bta_hh_handsk_act/bta_hh_ctrl_dat_act/bta_hh_cback jump tables, the 0x5C table
 *   0x80613340 that no relocation in the DOL reads) lies between this range's readers; the .sdata 0x80793D80 is read
 *   only by bta_hh_api_enable and bta_hh_sdp_cmpl. The cfg pointer 0x80793D58 that bta_hh_start_sdp reads stays in
 *   BTE/gki_time.c.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows: 0x8047F6C4, 0x8047F6E0 (between bta_hh_close_act and bta_hh_get_acl_q_info);
 *   names not pinned.
 * RESIDUALS. every body is unwritten: 0x8047E678-0x8047FDC0 (18 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
