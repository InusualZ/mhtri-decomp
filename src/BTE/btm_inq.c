/*
 * BTE/btm_inq.c - the BTM inquiry: discoverability and connectability, the inquiry/page scan types, inquiry mode,
 *   start and cancel inquiry, remote name requests, the inquiry database and the inquiry result/complete handlers.
 *
 * RANGE. .text 0x8048495C-0x804863F4 (25 functions, 0x1A98 B); .data 0x806141E0-0x806143F0; .sdata2
 *   0x8079D0B8-0x8079D0C0. Strong start: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806141DC ->
 *   0x806141E0) and BTM_SetDiscoverability, the first function of the BTE source's btm_inq.c, is its first reader.
 *   The end is medium: btm_inq_rmt_name_failed (the last btm_inq.c function in the BTE source) before btm_init; the
 *   next .data pad seam 0x806143F0 allows any cut in [0x8048610C, 0x80486EBC]. The .sdata2 words 0x8079D0B8 and
 *   0x8079D0BC (the GIAC and LIAC access codes) are read only by BTM_SetDiscoverability and btm_event_filter_complete.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x80484BE0 BTM_SetInquiryScanType and 0x80484C90
 *   BTM_SetPageScanType (twin bodies; the second calls btsnd_hcic_write_pagescan_type), at their BTE source positions
 *   after BTM_SetDiscoverability.
 * RESIDUALS. every body is unwritten: 0x8048495C-0x804863F4 (25 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
