/*
 * BTE/bte_logmsg.c - the trace output of the Broadcom BTE Bluetooth stack: a formatter that prints through OSReport
 *   and the LogMsg_0..LogMsg_6 entry points that call it unless tracing is off.
 *
 * RANGE. .text 0x8047A470-0x8047A610 (8 functions); .bss 0x80737960-0x80738140; .sdata 0x80793D68-0x80793D70;
 *   .sbss 0x80795028-0x8079502C. Strong start: 0x8047A470 formats into the 0x7E0 buffer 0x80737960 with vsprintf,
 *   appends "%s\n" (.sdata 0x80793D68) and prints it; LogMsg_0..LogMsg_6 tail-call it and read the trace switch
 *   0x80795028. Its .bss follows BTE/uusb_ppc.c's buffers, its .sdata BTE/bta_sys_cfg.c's pointer and its .sbss
 *   BTE/bte_hcisu.c's. Medium end: 0x8047A610, the alarm handler whose address only BTA_Init takes, and its fiber
 *   stack 0x80738140 go with BTE/bta_init.c as a static pair; the 4-byte .sdata hole 0x80793D6C before BTA_Init's
 *   0x80793D70 is this file's tail.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 8 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's. Unnamed row (GUESS, not applied): 0x8047A470 LogMsg, Broadcom's formatter of bte_logmsg.c.
 * RESIDUALS. every body is unwritten: 0x8047A470-0x8047A610 (8 functions).
 * SHAPES. plain C, variadic; the LogMsg_* declarations live in BTE/bte_logmsg.h.
 */

#include "BTE/bte_logmsg.h"
