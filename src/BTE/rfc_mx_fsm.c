/*
 * BTE/rfc_mx_fsm.c - the RFCOMM multiplexer state machine: rfc_mx_sm_execute, its per-state handlers and the L2CAP
 *   config replies.
 *
 * RANGE. .text 0x8049B314-0x8049BF58 (10 functions, 0xC44 B); .data 0x806176B8-0x80617970. The .text holds the BTE
 *   rfc_mx_fsm.c functions rfc_mx_sm_execute..rfc_mx_conf_ind; rfc_port_fsm.c's rfc_port_sm_execute follows. The .data
 *   holds the state machine's jump tables and trace strings, read only by this range's functions; no pad beyond 4-byte
 *   alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8049B314-0x8049BF58 (10 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
