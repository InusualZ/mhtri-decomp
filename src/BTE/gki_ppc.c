/*
 * BTE/gki_ppc.c - the GKI operating system glue of the Broadcom BTE Bluetooth stack on the Wii: GKI_init, the
 *   scheduler lock stubs, task events, the interrupt disable/enable pair and the pool memory allocator.
 *
 * RANGE. .text 0x80478D20-0x80479030 (13 functions); no data. Medium start: GKI_init (0x80478D20, calls
 *   gki_buffer_init and gki_timers_init) follows the timer list functions of BTE/gki_time.c, Broadcom's gki_time.c /
 *   gki_ppc.c order; no data on either side. Strong end: 0x80479030 is a static callback of BTE/hcisu_h2.c (its
 *   address is taken by 0x804795D0 there).
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 13 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's; GKI_send_event (0x80478EA0, sets an event bit of a task below 8 and returns 1 for a bad task),
 *   GKI_exception (0x80478FE0) and GKI_os_free (0x80479020, called by GKI_delete_pool) are the BTE source's names read
 *   from the bodies and their callers in BTE/gki_buffer.c. Unnamed rows (GUESS, not applied): 0x80478DB0
 *   GKI_shutdown (its only caller is BTA_CleanUp), 0x80478E60 GKI_run and 0x80478E90 GKI_delay (empty bodies BTA_Init
 *   calls with 0 and with 100 / 2000).
 * RESIDUALS. every body is unwritten: 0x80478D20-0x80479030 (13 functions).
 * SHAPES. plain C; the declarations live in BTE/gki_ppc.h.
 */

#include "BTE/gki_ppc.h"
