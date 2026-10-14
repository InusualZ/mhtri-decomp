/*
 * BTE/l2c_main.c - the L2CAP main module: l2c_init, the ACL receive path, the signalling command processor, the timeout
 *   dispatcher and the held-packet queue.
 *
 * RANGE. .text 0x804960C8-0x804970F8 (5 functions, 0x1030 B); .data 0x80616D18-0x80617018; .bss 0x8073C760-0x8073CF48.
 *   The start is medium confidence (BTE/l2c_link.c's header); the end is strong: the .data ends at the 4-byte pad
 *   before 0x80617018, whose readers are the l2cu_* family. The .bss is the L2CAP control block (0x7E8 B, read by every
 *   l2c unit and bta); its owner is a GUESS from the BTE source (l2c_main.c defines it) - any l2c unit gives the same
 *   .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804960C8-0x804970F8 (5 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
