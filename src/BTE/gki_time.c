/*
 * BTE/gki_time.c - the GKI timer API of the Broadcom BTE Bluetooth stack: the tick count, the task timers and the
 *   timer lists the BTU and the protocol layers link their entries into.
 *
 * RANGE. .text 0x80478790-0x80478D20 (8 functions); no data. Strong start: 0x80478790 (gki_timers_init) follows
 *   GKI_delete_pool, the last function of BTE/gki_buffer.c, whose .bss holds gki_cb, the control block these
 *   functions read. Medium end: GKI_init (0x80478D20) opens BTE/gki_ppc.c - Broadcom's gki_time.c ends with the timer
 *   list functions and the OS glue of gki_ppc.c follows; no data on either side.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). Every function start of 0x80478790-0x8047ABC4 is 16-aligned (61 of 61), as in
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC), while the GC-built files from 0x8047ABC4 on pack to 4 bytes:
 *   this head may be a different compiler band.
 * NAMES. the map's. Unnamed row (GUESS, not applied): 0x80478A50 GKI_init_timer_list (zeroes the three words of a
 *   list head; its only caller is ptim_init in BTE/ptim.c). GUESS BTA_DisableBluetooth (0x8047D718 in
 *   BTE/bta_dm_api.c, declared in BTE/gki_time.h: the bta_dm_api entry after BTA_EnableBluetooth, called from the WUD
 *   shutdown fiber).
 * RESIDUALS. every body is unwritten: 0x80478790-0x80478D20 (8 functions).
 * SHAPES. plain C; BTE/gki_time.h holds the timer list entry type and includes the headers of the files cut from
 *   this head.
 */

#include "BTE/gki_time.h"
