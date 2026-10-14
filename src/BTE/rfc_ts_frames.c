/*
 * BTE/rfc_ts_frames.c - the RFCOMM frame builders and parser: SABME/UA/DM/DISC/UIH frames, the multiplexer commands,
 *   rfc_parse_data and rfc_process_mx_message.
 *
 * RANGE. .text 0x8049D624-0x8049EC6C (15 functions, 0x1648 B); .data 0x80617C98-0x80617D70; .sdata
 *   0x80793DB8-0x80793DC8. The .text starts at 0x8049D624, not at rfc_send_dm: the two 0xB0 B functions before
 *   rfc_send_dm build the same 4-byte control frame with control bytes 0x3F (SABME|P) and 0x73 (UA|F), the 0x8049D848
 *   one with 0x53 (DISC|P), all via rfc_calc_fcs and rfc_check_send_cmd - the BTE rfc_ts_frames.c order rfc_send_sabme,
 *   rfc_send_ua, rfc_send_dm, rfc_send_disc. It ends with rfc_process_mx_message; rfc_utils.c's rfc_calc_fcs follows.
 *   The .data and .sdata are read only by this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed rows (GUESS, not applied): 0x8049D624 rfc_send_sabme, 0x8049D6D4 rfc_send_ua and
 *   0x8049D848 rfc_send_disc (by their control bytes), 0x8049DF6C rfc_send_test (by source order).
 * RESIDUALS. every body is unwritten: 0x8049D624-0x8049EC6C (15 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
