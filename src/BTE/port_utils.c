/*
 * BTE/port_utils.c - the RFCOMM port utilities: MTU selection, port release and lookup, user and peer flow control.
 *
 * RANGE. .text 0x8049A408-0x8049A9E0 (7 functions, 0x5D8 B); .data 0x80617430-0x80617578. The .text holds the BTE
 *   port_utils.c functions port_select_mtu..port_flow_control_peer; rfc_l2cap_if.c's rfcomm_l2cap_if_init follows. The
 *   .data starts after a 4-byte pad beyond 4-byte alignment (0x8061742C -> 0x80617430: a new 8-aligned contribution).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8049A408-0x8049A9E0 (7 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
