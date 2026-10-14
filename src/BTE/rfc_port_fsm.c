/*
 * BTE/rfc_port_fsm.c - the RFCOMM port state machine: rfc_port_sm_execute, its per-state handlers and the
 *   PN/RPN/MSC/RLS/FCON/FCOFF processing, plus L2CAP congestion and port state set.
 *
 * RANGE. .text 0x8049BF58-0x8049D0F8 (17 functions, 0x11A0 B); .data 0x80617970-0x80617C98. The .text holds the BTE
 *   rfc_port_fsm.c functions rfc_port_sm_execute..rfc_set_port_state; rfc_port_if.c's RFCOMM_StartRsp follows. The .data
 *   (jump tables and trace strings) is read only by this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed rows, by the BTE source order only (GUESS, not applied): 0x8049CEF8 rfc_process_nsc and
 *   0x8049CEFC rfc_process_test (4 and 8 B).
 * RESIDUALS. every body is unwritten: 0x8049BF58-0x8049D0F8 (17 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
