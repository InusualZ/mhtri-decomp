/*
 * BTE/btu_task1.c - the BTU task of the Broadcom BTE Bluetooth stack: its initialisation, the task body that serves
 *   the HCI and protocol mailboxes and the timer list, and the BTU timer start and stop.
 *
 * RANGE. .text 0x8047A780-0x8047ABC4 (4 functions); .bss 0x80739170-0x807391F8; .sdata 0x80793D78-0x80793D80;
 *   .sbss 0x80795030-0x80795038. Strong start: btu_task_init (0x8047A780) initialises the .sdata 0x80793D78 /
 *   0x80793D7C and the .sbss 0x80795030 / 0x80795034 the task body 0x8047A7C0 reads. Strong end: 0x8047ABC4 is the
 *   first function start of the BTE band that is not 16-aligned (BTE/bd.c). The BTU control block btu_cb (.bss
 *   0x80739170, read by the task body, the timer functions and BTE/hcisu_h2.c, cleared by BTE_Init in
 *   BTE/btu_init.c) lies after BTE/bta_init.c's alarm and before the bta_sys control block 0x807391F8 of the files
 *   that follow, so it is this file's.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 4 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's. Unnamed row (GUESS, not applied): 0x8047A7C0 btu_task, the fiber body BTE/bta_init.c runs.
 * RESIDUALS. every body is unwritten: 0x8047A780-0x8047ABC4 (4 functions).
 * SHAPES. plain C; btu_cb, its type and the timer declarations live in BTE/btu_task1.h.
 */

#include "BTE/btu_task1.h"
