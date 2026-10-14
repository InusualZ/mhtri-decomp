/*
 * BTE/btm_devctl.c - the BTM device control: device init and reset, the HCI reset sequence and its completions, AFH
 *   channels, local name/address/version/features/class, the device status notification and the vendor-specific
 *   commands and events.
 *
 * RANGE. .text 0x80482EA0-0x80484224 (24 functions, 0x1384 B); .data 0x80613F80-0x806140E0; .sdata
 *   0x80793D88-0x80793D8C. The start is medium: btm_dev_init after btm_dev.c in the BTE source order; the .data start
 *   0x80613F80 is 8-aligned with no pad (first readers BTM_SetAfhChannels and btm_read_local_features_complete). The
 *   end is strong: the next .data object starts after a 4-byte pad beyond 4-byte alignment (0x806140DC ->
 *   0x806140E0), and its first reader BTM_WritePageTimeout is the first function after btm_vendor_specific_evt, the
 *   last reader of this range's .data - the cut is pinned exactly at 0x80484224. The .sdata word 0x80793D88 is read
 *   only by btm_dev_init.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x80483134 BTM_IsDeviceUp (compares the btm_cb device state
 *   byte with 5), 0x80483FB4 BTM_ReadDeviceClass (returns the address of the btm_cb device class), both at their BTE
 *   source positions.
 * RESIDUALS. every body is unwritten: 0x80482EA0-0x80484224 (24 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
