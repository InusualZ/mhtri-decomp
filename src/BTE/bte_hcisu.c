/*
 * BTE/bte_hcisu.c - the glue between the BTU task and the HCI services of the Broadcom BTE Bluetooth stack: sends,
 *   polls and closes through the installed transport interface, and posts received messages to the BTU task.
 *
 * RANGE. .text 0x8047A370-0x8047A470 (5 functions); .sbss 0x80795020-0x80795028. Strong start: bte_hcisu_send
 *   (0x8047A370) is the first reader of the interface pointer 0x80795020 and its context 0x80795024 (written by
 *   BTA_Init), which follow BTE/uusb_ppc.c's .sbss. Strong end: 0x8047A470 is the formatter the LogMsg_* functions
 *   tail-call, whose buffer and "%s\n" are BTE/bte_logmsg.c's data. 0x8047A430 (posts a message to the BTU task's
 *   mailbox 0) is the receive callback whose address the USB transport takes.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 5 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8047A370-0x8047A470 (5 functions). BTE_InitStack (0x8047A440) is Broadcom's
 *   bte_init.c function; it references no data, so a separate cut 0x8047A440-0x8047A470 is neither proven nor
 *   refuted by the bytes.
 * SHAPES. plain C; the declarations live in BTE/bte_hcisu.h.
 */

#include "BTE/bte_hcisu.h"
