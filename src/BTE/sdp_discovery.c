/*
 * BTE/sdp_discovery.c - the SDP client discovery: request building, the server response dispatch and the response
 *   parsers that fill the discovery database.
 *
 * RANGE. .text 0x804A0F80-0x804A21F8 (10 functions, 0x1278 B); .data 0x80617EE8-0x80618048. The .text holds the BTE
 *   sdp_discovery.c functions sdpu_build_uuid_seq..add_attr; sdp_main.c's sdp_init follows. The .data is read only by
 *   this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed row, by the BTE source order only (GUESS, not applied): 0x804A119C sdp_disc_connected.
 * RESIDUALS. every body is unwritten: 0x804A0F80-0x804A21F8 (10 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
