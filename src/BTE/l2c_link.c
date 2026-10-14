/*
 * BTE/l2c_link.c - the L2CAP link layer: HCI connection events, link timeouts, the ACL send path and buffer allocation,
 *   packet-start/end checks, role changes and the PIN code request.
 *
 * RANGE. .text 0x80494F3C-0x804960C8 (17 functions, 0x118C B); .data 0x80616AA8-0x80616D18. The start is pinned by the
 *   .data readers; the end is medium confidence: the 4-byte pad before 0x80616D18 (0x80616D14 -> 0x80616D18) allows any
 *   cut in [0x80496084, 0x804961DC], and the BTE source keeps l2c_pin_code_request in l2c_link.c and l2c_init in
 *   l2c_main.c. The 4-byte pad inside the .data at 0x80616AF0 is not a seam: the string at 0x80616AA8 is read by both
 *   l2c_link_hci_conn_req and l2c_link_check_send_pkts, one pooled literal.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80494F3C-0x804960C8 (17 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
