/*
 * BTE/sdp_api.c - the SDP client API: discovery database setup, service search requests, record and attribute lookups,
 *   the device identification record and the trace level.
 *
 * RANGE. .text 0x8049F44C-0x804A02B4 (9 functions, 0xE68 B); .data 0x80617E00-0x80617E50. The .text holds the BTE
 *   sdp_api.c functions SDP_InitDiscoveryDb..SDP_SetTraceLevel; sdp_db.c's sdp_db_service_search follows. The .data
 *   starts after a 4-byte pad beyond 4-byte alignment (0x80617DFC -> 0x80617E00).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8049F44C-0x804A02B4 (9 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
