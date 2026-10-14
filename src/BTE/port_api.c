/*
 * BTE/port_api.c - the RFCOMM port API: RFCOMM_Init, which clears the RFCOMM control block and calls
 *   rfcomm_l2cap_if_init.
 *
 * RANGE. .text 0x80498FBC-0x8049900C (1 function, 0x50 B); .bss 0x8073CF48-0x8073D360. RFCOMM_Init is the last
 *   function of the BTE port_api.c and the only one retail keeps (the PORT_* API is dead-stripped); port_rfc.c's
 *   PORT_StartCnf follows. The .bss is the RFCOMM control block (0x418 B, read by every RFCOMM unit); its owner is a
 *   GUESS - any RFCOMM unit before sdp_main gives the same .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80498FBC-0x8049900C (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
