/*
 * BTE/gap_api.c - the GAP API: GAP_Init, which clears the GAP control block, registers the two BTM callbacks and calls
 *   gap_conn_init.
 *
 * RANGE. .text 0x8048C294-0x8048C2F4 (1 function, 0x60 B); .bss 0x8073BE60-0x8073C210. GAP_Init is the only GAP API
 *   function retail keeps; gap_conn.c's gap_conn_init follows. Medium confidence on the .text (no data on either side).
 *   The .bss is the GAP control block (0x3B0 B, read by GAP_Init and the gap_conn/gap_utils functions, after the BTM
 *   control block and before the HID device one); its owner is a GUESS from the BTE source (gap_api.c defines it) - any
 *   GAP unit gives the same .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048C294-0x8048C2F4 (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
