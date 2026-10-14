/*
 * BTE/hidd_api.c - the HID device API: HID_DevInit, which clears the HID device control block and arms its repage timer
 *   callback.
 *
 * RANGE. .text 0x8048FB68-0x8048FBD0 (1 function, 0x68 B); .bss 0x8073C210-0x8073C358. Medium confidence: HID_DevInit
 *   alone between the last btsnd_hcic_* builder and hidd_conn_initiate, no data on either side. The .bss is the HID
 *   device control block (0x148 B, read by the three hidd units); its owner is a GUESS from the BTE source (hidd_api.c
 *   defines it) - any hidd unit gives the same .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048FB68-0x8048FBD0 (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
