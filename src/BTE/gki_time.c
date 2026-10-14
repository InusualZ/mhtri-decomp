/*
 * BTE/gki_time.c - the rest of the Broadcom BTE Bluetooth stack after the GKI buffer file: GKI timers and OS glue,
 *   hcisu, the USB transport, bte_main, btu, bta_sys/bta_dm/bta_hh, btm, gap, hci, hidd/hidh, l2c, port/rfc and sdp.
 *
 * RANGE. .text 0x80478790-0x804A4AC0 (712 functions); .rodata 0x80573530-0x805739A8; .data 0x80612E60-0x806184E0;
 *   .bss 0x807358C0-0x80741998; .sdata 0x80793D28-0x80793DC8; .sbss 0x80795008-0x80795040; .sdata2
 *   0x8079D080-0x8079D0D8; .sbss2 0x8079D7E0-0x8079D7F0. The remainder of the old BTE band 0x804772F0-0x804A4AC0
 *   once BTE/gki_buffer.c was carved off its head; it is still several source files (gki_time, gki_common, hcisu_h2,
 *   uusb, bte_main, btu_task, bta_sys_*, bta_dm_*, bta_hh_*, btm_*, l2c_*, sdp_* ...), cut later one file at a time.
 * FLAGS. the `OS` lib default `cflags_os` (4-byte function alignment). The band is mixed: every function start from
 *   0x80478790 up to 0x8047ABC4 is 16-aligned (the `cflags_base` packing BTE/gki_buffer.c takes), while 488 of the
 *   651 starts from 0x8047ABC4 on are not - the alignment scan of the map's function rows. The file that ends at
 *   0x8047ABC4 is a seam: the 16-aligned head moves to `cflags_base` when it is cut out.
 * NAMES. the map's; GKI_send_event (0x80478EA0, sets an event bit of a task below 8 and returns 1 for a bad task),
 *   GKI_exception (0x80478FE0) and GKI_os_free (0x80479020, called by GKI_delete_pool) are the BTE source's names read
 *   from the bodies and their callers in BTE/gki_buffer.c. GUESS BTA_DisableBluetooth (0x8047D718: the bta_dm_api entry after
 *   BTA_EnableBluetooth, called from the WUD shutdown fiber).
 * RESIDUALS. every body is unwritten: 0x80478790-0x804A4AC0 (712 functions, the largest the BTA/BTM state machines).
 * SHAPES. seams measured so far: 0x80478790 (gki_timers_init, the first function after GKI_delete_pool) and
 *   0x8047ABC4 (the first 4-aligned start).
 */

#include "BTE/gki_time.h"
