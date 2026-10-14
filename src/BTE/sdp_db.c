/*
 * BTE/sdp_db.c - the SDP server database: service search, record and attribute lookup, record creation and deletion,
 *   attribute adds and deletes.
 *
 * RANGE. .text 0x804A02B4-0x804A0F80 (10 functions, 0xCCC B); .data 0x80617E50-0x80617EE8. The .text holds the BTE
 *   sdp_db.c functions sdp_db_service_search..SDP_DeleteAttribute; sdp_discovery.c's sdpu_build_uuid_seq follows. The
 *   .data is read only by this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804A02B4-0x804A0F80 (10 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
