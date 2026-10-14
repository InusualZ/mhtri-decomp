/*
 * BTE/hidh_api.c - the HID host API: the SDP record query and its search callback, init, register/deregister,
 *   add/remove/open/write/close device and the security level.
 *
 * RANGE. .text 0x804900D4-0x80490E54 (11 functions, 0xD80 B); .data 0x806155E0-0x806156F8; .bss 0x8073C358-0x8073C760.
 *   Strong: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806155DC -> 0x806155E0) and the BTE
 *   hidh_api.c opens with HID_HostGetSDPRecord and its static hidh_search_callback; it ends with
 *   HID_HostSetSecurityLevel, pinned by the .data (hidh_conn.c's first reader is hidh_conn_reg at 0x80490E54). The .bss
 *   is the HID host control block (0x408 B, read by both hidh units and bta_hh); its owner is a GUESS from the BTE
 *   source (hidh_api.c defines it) - either hidh unit gives the same .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804900D4-0x80490E54 (11 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
