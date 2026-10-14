/*
 * BTE/btm_link_key.c - the BTM page timeout and stored link key commands: BTM_WritePageTimeout, the read/write/delete
 *   stored link key requests and their completions, the returned link keys event and the device status report.
 *
 * RANGE. .text 0x80484224-0x80484828 (9 functions, 0x604 B); .data 0x806140E0-0x806141E0; .sdata
 *   0x80793D8C-0x80793DA0. Strong start: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806140DC ->
 *   0x806140E0) and BTM_WritePageTimeout is its first reader, the first function after btm_devctl.c's last .data
 *   reader. The end is weak: the next .data pad seam (0x806141E0, BTM_SetDiscoverability) allows any cut in
 *   [0x8048453C, 0x8048495C]; it is placed after btm_report_device_status, before the btm_discovery_db_* family. The
 *   .sdata strings "TRUE"/"FALSE" (0x80793D8C, 0x80793D94) are read only by BTM_ReadStoredLinkKey and
 *   BTM_DeleteStoredLinkKey.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. The file name is a GUESS from its functions: the later BTE source keeps them in btm_devctl.c, but
 *   the .data pad seam puts them in a separate file whose name the binary does not carry.
 * RESIDUALS. every body is unwritten: 0x80484224-0x80484828 (9 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
