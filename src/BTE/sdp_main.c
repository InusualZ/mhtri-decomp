/*
 * BTE/sdp_main.c - the SDP main module: sdp_init and the L2CAP callbacks (connect, config, disconnect, data),
 *   connection origination and the connection timeout.
 *
 * RANGE. .text 0x804A21F8-0x804A2D64 (11 functions, 0xB6C B); .data 0x80618048-0x806183A8; .bss 0x8073D360-0x80741998.
 *   The .text holds the BTE sdp_main.c functions sdp_init..sdp_conn_timeout; sdp_server.c's sdp_server_handle_client_req
 *   follows. The .data starts after a 4-byte pad beyond 4-byte alignment (0x80618044 -> 0x80618048). The .bss is the SDP
 *   control block (0x4638 B, the last BTE .bss object, cleared by sdp_init).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804A21F8-0x804A2D64 (11 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
