/*
 * BTE/bd.c - the Bluetooth device address helpers: a 6-byte address copy and a 6-byte address compare.
 *
 * RANGE. .text 0x8047ABC4-0x8047AC98 (2 functions, 0xD4 B); no data. Strong start: 0x8047ABC4 is the first function
 *   start of the BTE band that is not 16-aligned (all 61 starts from 0x80478790 are), the alignment seam that ends the
 *   BTE/gki_time.c head. Medium end: BTE/bta_sys_conn.c's callback registrations follow, the first functions that write
 *   the bta_sys control block. The all-zero address 0x8079D7E8 (.sbss2), passed by address to 0x8047ABF8 by
 *   bta_hh_find_cb, sits with its only reader in BTE/bta_hh_utils.c; Broadcom's bd.c defines a bd_addr_null of that
 *   shape, and placing it here instead gives the same layout.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x8047ABC4 bdcpy (copies 6 bytes from r4 to r3), 0x8047ABF8
 *   bdcmp (compares 6 bytes, returns 0 or -1), the Broadcom bd.c names.
 * RESIDUALS. every body is unwritten: 0x8047ABC4-0x8047AC98 (2 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
