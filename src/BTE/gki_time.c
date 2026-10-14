/*
 * BTE/gki_time.c - the Broadcom BTE Bluetooth stack between the GKI buffer file and the BTM ACL file: GKI timers and OS
 *   glue, hcisu, the USB transport, bte_main, btu_task and bta_sys/bta_dm/bta_hh.
 *
 * RANGE. .text 0x80478790-0x80480A90 (195 functions); .rodata 0x80573530-0x80573840; .data 0x80612E60-0x80613B60;
 *   .bss 0x807358C0-0x80739698; .sdata 0x80793D28-0x80793D88; .sbss 0x80795008-0x80795040; .sdata2
 *   0x8079D080-0x8079D0B8; .sbss2 0x8079D7E0-0x8079D7F0. The old BTE band 0x804772F0-0x804A4AC0 between
 *   BTE/gki_buffer.c (its head) and the per-file units BTE/btm_acl.c..BTE/sdp_utils.c (its tail, from btm_acl_init at
 *   0x80480A90); it is still several source files (gki_time, gki_common, hcisu_h2, uusb, bte_main, btu_task,
 *   bta_sys_*, bta_dm_*, bta_hh_* ...), cut later one file at a time. The .bss keeps the BTU control block
 *   (0x80739170, cleared by BTE_Init in BTE/btu_init.c), which sits in the .bss order of the head files.
 * FLAGS. the `OS` lib default `cflags_os` (4-byte function alignment). The band is mixed: every function start from
 *   0x80478790 up to 0x8047ABC4 is 16-aligned (the `cflags_base` packing BTE/gki_buffer.c takes), while 98 of the
 *   134 starts from 0x8047ABC4 on are not - the alignment scan of the map's function rows. The file that ends at
 *   0x8047ABC4 is a seam: the 16-aligned head moves to `cflags_base` when it is cut out.
 * NAMES. the map's; GKI_send_event (0x80478EA0, sets an event bit of a task below 8 and returns 1 for a bad task),
 *   GKI_exception (0x80478FE0) and GKI_os_free (0x80479020, called by GKI_delete_pool) are the BTE source's names read
 *   from the bodies and their callers in BTE/gki_buffer.c. GUESS BTA_DisableBluetooth (0x8047D718: the bta_dm_api entry after
 *   BTA_EnableBluetooth, called from the WUD shutdown fiber).
 * RESIDUALS. every body is unwritten: 0x80478790-0x80480A90 (195 functions, the largest the BTA state machines).
 * SHAPES. seams measured so far: 0x80478790 (gki_timers_init, the first function after GKI_delete_pool),
 *   0x8047ABC4 (the first 4-aligned start) and 0x80480A90 (btm_acl_init, the first BTE/btm_acl.c function).
 */

#include "BTE/gki_time.h"
