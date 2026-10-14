/*
 * BTE/sdp_utils.c - the SDP utilities: CCB allocation and release, attribute sequence building, error responses, UID
 *   and attribute sequence extraction, UUID comparison and attribute list sorting.
 *
 * RANGE. .text 0x804A3B04-0x804A4AC0 (12 functions, 0xFBC B); .rodata 0x80573998-0x805739A8; .data
 *   0x806183E8-0x806184E0. The .text holds the BTE sdp_utils.c functions sdpu_allocate_ccb..sdpu_sort_attr_list and ends
 *   at DB/db.c's 16-aligned DBInit (0x804A4AC0). The .rodata is the 16 B Bluetooth base UUID; the .data opens with a
 *   jump table and is read only by this range's functions, no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804A3B04-0x804A4AC0 (12 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
