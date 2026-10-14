/*
 * BTE/btu_init.c - the BTU stack initialisation: the core init that starts btm, l2c, sdp and the discovery database,
 *   and BTE_Init, which clears the BTU control block.
 *
 * RANGE. .text 0x8048C13C-0x8048C1B4 (2 functions, 0x78 B); no data. 0x8048C13C calls btm_init, l2c_init, sdp_init and
 *   btm_discovery_db_init and is called from btu_task_init: the BTE btu_init.c btu_init_core, which precedes BTE_Init
 *   in that file. Medium confidence: no data on either side of 0x8048C13C, so the start rests on the two bodies;
 *   wbt_ext.c's WBT_ExtCreateRecord follows. The BTU control block BTE_Init clears (.bss 0x80739170) sits in the .bss
 *   order of BTE/gki_time.c's head files and stays there.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's. GUESS btu_init_core (0x8048C13C, by its callees), not applied.
 * RESIDUALS. every body is unwritten: 0x8048C13C-0x8048C1B4 (2 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
