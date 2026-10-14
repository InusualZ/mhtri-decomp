/*
 * BTE/hidd_conn.c - the HID device connection layer: hidd_conn_initiate and the repage timeout that retries it.
 *
 * RANGE. .text 0x8048FBD0-0x8048FD58 (2 functions, 0x188 B); .data 0x80615590-0x806155E0. Medium confidence on the
 *   .text start (HID_DevInit reads no data); the end is hidd_pm.c's hidd_pm_set_now. The .data (trace strings read only
 *   by these two functions) starts with no pad beyond 4-byte alignment where the readers change and ends at the 4-byte
 *   pad before 0x806155E0. Both starts are 16-aligned with no padding before either (0x8048FBD0 + 0xC0 = 0x8048FC90).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048FBD0-0x8048FD58 (2 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
