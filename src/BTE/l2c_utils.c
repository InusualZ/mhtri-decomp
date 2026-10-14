/*
 * BTE/l2c_utils.c - the L2CAP utilities: link and channel control block allocation, release and lookup, the peer
 *   signalling builders, configuration processing and connection creation.
 *
 * RANGE. .text 0x804970F8-0x80498FBC (33 functions, 0x1EC4 B); .data 0x80617018-0x80617100; .sdata
 *   0x80793DA8-0x80793DB8. Strong: the .data starts after a 4-byte pad beyond 4-byte alignment (0x80617014 ->
 *   0x80617018) and the l2cu_* family starts at 0x804970F8; it ends with l2cu_lcb_disconnecting, before port_api.c's
 *   RFCOMM_Init. The .data and .sdata (the TRUE/FALSE strings) are read only by this range's functions.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. GUESS (not applied): 0x80497354 l2cu_find_lcb_by_handle (after l2cu_find_lcb_by_bd_addr, reads the
 *   L2CAP control block, called with a handle by the link and receive paths), 0x80498850 l2cu_release_rcb (between
 *   l2cu_allocate_rcb and l2cu_find_rcb_by_psm, called by L2CA_Deregister). Unnamed rows: 0x804973E4, 0x80498E44.
 * RESIDUALS. every body is unwritten: 0x804970F8-0x80498FBC (33 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
