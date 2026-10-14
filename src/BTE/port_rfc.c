/*
 * BTE/port_rfc.c - the RFCOMM port layer's indications and confirmations from the RFCOMM state machines (PORT_StartCnf
 *   .. port_rfc_closed).
 *
 * RANGE. .text 0x8049900C-0x8049A408 (18 functions, 0x13FC B); .data 0x80617100-0x80617430. The .text holds the BTE
 *   port_rfc.c functions PORT_StartCnf..port_rfc_closed; port_utils.c's port_select_mtu follows. The .data (trace
 *   strings) starts after the last BTE/gki_time.c string with no pad beyond 4-byte alignment: the seam is where the
 *   referrers change (every object is read only by this range's functions).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8049900C-0x8049A408 (18 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
