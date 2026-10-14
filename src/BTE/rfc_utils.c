/*
 * BTE/rfc_utils.c - the RFCOMM utilities: FCS calculation and check, multiplexer channel allocation, timers, security
 *   completion, port close and credit flow control.
 *
 * RANGE. .text 0x8049EC6C-0x8049F44C (15 functions, 0x7E0 B); .rodata 0x80573898-0x80573998; .data
 *   0x80617D70-0x80617E00. The .text holds the BTE rfc_utils.c functions rfc_calc_fcs..rfc_check_send_cmd; sdp_api.c's
 *   SDP_InitDiscoveryDb follows. The .rodata is the 0x100 B FCS table rfc_calc_fcs reads; the .data is read only by this
 *   range's functions, no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8049EC6C-0x8049F44C (15 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
