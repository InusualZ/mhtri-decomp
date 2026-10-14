/*
 * BTE/l2c_csm.c - the L2CAP channel state machine: l2c_csm_execute, one handler per channel state, and the peer-data
 *   forwarder.
 *
 * RANGE. .text 0x80493A60-0x80494F3C (11 functions, 0x14DC B); .data 0x80616270-0x80616AA8. The .text holds the BTE
 *   l2c_csm.c functions l2c_csm_execute..forward_peer_data; the end is pinned by the .data (forward_peer_data is the
 *   last reader below 0x80616AA8, l2c_link_hci_conn_req the first above). The .data (state switch tables and trace
 *   strings) starts with no pad beyond 4-byte alignment where the readers change.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80493A60-0x80494F3C (11 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
