/*
 * BTE/rfc_l2cap_if.c - the RFCOMM L2CAP interface: the L2CAP callback registration, connect/config/disconnect
 *   indications, data and congestion callbacks, and the LCID-to-multiplexer table store.
 *
 * RANGE. .text 0x8049A9E0-0x8049B314 (10 functions, 0x934 B); .data 0x80617578-0x806176B8. The .text holds the BTE
 *   rfc_l2cap_if.c functions rfcomm_l2cap_if_init..RFCOMM_CongestionStatusInd and rfc_save_lcid_mcb, which closes the
 *   BTE file; rfc_mx_fsm.c's rfc_mx_sm_execute follows at 0x8049B314. The rfc_save_lcid_mcb placement is medium
 *   confidence: it reads only the RFCOMM control block and its two callers are both in BTE/rfc_mx_fsm.c, so only the BTE
 *   source order puts it here. The .data is one table read only by this range's functions; no pad beyond 4-byte
 *   alignment.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. Unnamed rows, identified by the BTE source order only (GUESS, not applied): 0x8049AA5C
 *   RFCOMM_ConnectInd, 0x8049ABEC RFCOMM_ConfigInd, 0x8049ACF4 RFCOMM_ConfigCnf, 0x8049ADFC RFCOMM_QoSViolationInd (an
 *   empty body).
 * RESIDUALS. every body is unwritten: 0x8049A9E0-0x8049B314 (10 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
