/*
 * BTE/bta_init.c - the start and stop of the Broadcom BTE Bluetooth stack on the Wii: BTA_Init brings up GKI, the
 *   BTE core and the HCI transport and drives the BTU task from a periodic alarm; BTA_CleanUp stops it.
 *
 * RANGE. .text 0x8047A610-0x8047A780 (4 functions); .bss 0x80738140-0x80739170; .sdata 0x80793D70-0x80793D78;
 *   .sbss 0x8079502C-0x80795030. Medium start: 0x8047A610 switches to the fiber stack 0x80738140 (0x1000 bytes) and
 *   runs the BTU task; its address is taken only by BTA_Init, which installs it on the periodic alarm 0x80739140,
 *   and its stack lies directly before that alarm in the .bss. Strong .sdata seam: BTA_Init's 0x80793D70 follows
 *   the 4-byte hole after BTE/bte_logmsg.c's "%s\n". Strong end: btu_task_init (0x8047A780) is the first reader of
 *   BTE/btu_task1.c's .sdata 0x80793D78 and .sbss 0x80795030. The cleanup callback 0x8079502C is set by BTA_CleanUp
 *   and called by bta_usb_close_evt.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 4 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's; the file name is a GUESS from BTA_Init.
 * RESIDUALS. every body is unwritten: 0x8047A610-0x8047A780 (4 functions).
 * SHAPES. plain C; the declarations live in BTE/bta_init.h.
 */

#include "BTE/bta_init.h"
