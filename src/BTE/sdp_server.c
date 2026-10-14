/*
 * BTE/sdp_server.c - the SDP server: the client request dispatch and the service search, attribute and
 *   search-attribute handlers.
 *
 * RANGE. .text 0x804A2D64-0x804A3B04 (5 functions, 0xDA0 B); .data 0x806183A8-0x806183E8. The .text holds the BTE
 *   sdp_server.c functions sdp_server_handle_client_req..process_service_search_attr_req and one unnamed 0x90 B function
 *   after them; sdp_utils.c's sdpu_allocate_ccb follows. The .data starts after a 4-byte pad beyond 4-byte alignment
 *   (0x806183A4 -> 0x806183A8).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's; 0x804A3A74 is unnamed and unidentified.
 * RESIDUALS. every body is unwritten: 0x804A2D64-0x804A3B04 (5 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
