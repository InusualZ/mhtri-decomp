/*
 * BTE/btu_hcif.c - the BTU HCI interface: the HCI event dispatcher, command send and queueing, and the per-event
 *   handlers (connection, QoS, eSCO, command complete/status, command timeout, link key notification).
 *
 * RANGE. .text 0x8048AEE0-0x8048C13C (12 functions, 0x125C B); .data 0x80615140-0x806151D0. The .text holds the BTE
 *   btu_hcif.c functions btu_hcif_process_event..btu_hcif_link_key_notification_evt; btu_init.c's btu_init_core
 *   follows. The start is pinned by the .data: the last BTE/gki_time.c .data reader is 0x8048ADCC and
 *   btu_hcif_process_event is the first reader of 0x80615140 (no pad beyond 4-byte alignment, so the seam is where the
 *   readers change). Every .data object is read only by this range's functions.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048AEE0-0x8048C13C (12 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
