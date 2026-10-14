/*
 * BTE/wbt_ext.c - the WBT extension: WBT_ExtCreateRecord, which adds the device identification attributes to the local
 *   SDP DI record.
 *
 * RANGE. .text 0x8048C1B4-0x8048C294 (1 function, 0xE0 B); no data. Medium confidence: the only function of its family,
 *   between BTE_Init and GAP_Init, with no data on either side.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048C1B4-0x8048C294 (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
