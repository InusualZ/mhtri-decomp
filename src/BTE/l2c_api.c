/*
 * BTE/l2c_api.c - the L2CAP API: L2CA_Register/Deregister, connect/config/disconnect requests and responses, data
 *   write, the idle timeouts and the trace level.
 *
 * RANGE. .text 0x80492E94-0x80493A60 (13 functions, 0xBCC B); .data 0x80615AE0-0x80616270. Strong: the .data starts
 *   after a 4-byte pad beyond 4-byte alignment (0x80615ADC -> 0x80615AE0), pinning the start to [0x80492E68,
 *   0x80492E94] - L2CA_Register, after hidh_conn_dereg. It ends with an empty 4-byte function at 0x80493A5C (called by
 *   bta_dm_enable); l2c_csm.c opens with l2c_csm_execute at 0x80493A60. Medium confidence on the empty function's side:
 *   the .data allows any cut in [0x80493940, 0x80493A60], and only the BTE source order (l2c_csm.c's first function is
 *   l2c_csm_execute) puts it here.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed row: 0x80493A5C (an empty entry point called by bta_dm_enable).
 * RESIDUALS. every body is unwritten: 0x80492E94-0x80493A60 (13 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
