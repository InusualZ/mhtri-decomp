/*
 * BTE/gki_time.c - the 16-aligned head of the Broadcom BTE Bluetooth stack between the GKI buffer file and BTE/bd.c:
 *   GKI timers and OS glue, hcisu, the USB transport, bte_hcisu, the log messages, BTA_Init and btu_task, with the BTA
 *   configuration data.
 *
 * RANGE. .text 0x80478790-0x8047ABC4 (61 functions); .rodata 0x80573530-0x805735E8; .data 0x80612E60-0x80612ED0;
 *   .bss 0x807358C0-0x807391F8; .sdata 0x80793D28-0x80793D80; .sbss 0x80795008-0x80795038; .sdata2
 *   0x8079D080-0x8079D098; .sbss2 0x8079D7E0-0x8079D7E8. The old BTE band 0x804772F0-0x804A4AC0 between
 *   BTE/gki_buffer.c (its head) and the per-file units BTE/bd.c..BTE/sdp_utils.c (its tail, from the address copy at
 *   0x8047ABC4); it is still several source files (gki_time, gki_ppc, hcisu_h2, uusb, bte_hcisu, bte_logmsg, bta_init,
 *   btu_task), cut later one file at a time. The .bss keeps the BTU control block (0x80739170, cleared by BTE_Init in
 *   BTE/btu_init.c), which sits in the .bss order of the head files.
 *   The BTA configuration files (Broadcom's bta_dm_cfg, bta_hh_cfg and bta_sys_cfg, data only) stay here: the device
 *   config .rodata 0x80573548 (read by bta_dm_enable and the power manager), the tables 0x80573554-0x805735D8 behind
 *   the .sdata pointers 0x80793D38-0x80793D48 (resource manager, compression, power manager) and the zero table
 *   .sbss2 0x8079D7E0 behind 0x80793D40; the HID device table .sdata 0x80793D50 and its config .rodata 0x805735D8
 *   behind 0x80793D58; the system config .sdata2 0x8079D090 behind 0x80793D60. Their .sdata lies before bte_hcisu's
 *   0x80793D68 while their .rodata lies after 0x80573530 (read by BTA_Init), so no file order after this head holds
 *   both; with 0x80573530 in a file up to the USB transport, the three sit between the USB transport and bte_hcisu in
 *   every section, and are cut with the head.
 * FLAGS. the `OS` lib default `cflags_os` (4-byte function alignment). Every one of the 61 function starts is
 *   16-aligned (the `cflags_base` packing BTE/gki_buffer.c takes), while 98 of the 134 starts of
 *   BTE/bd.c..BTE/bta_hh_utils.c that follow are not - the alignment scan of the map's function rows. This file moves
 *   to `cflags_base` when it is cut.
 * NAMES. the map's; GKI_send_event (0x80478EA0, sets an event bit of a task below 8 and returns 1 for a bad task),
 *   GKI_exception (0x80478FE0) and GKI_os_free (0x80479020, called by GKI_delete_pool) are the BTE source's names read
 *   from the bodies and their callers in BTE/gki_buffer.c. GUESS BTA_DisableBluetooth (0x8047D718 in BTE/bta_dm_api.c:
 *   the bta_dm_api entry after BTA_EnableBluetooth, called from the WUD shutdown fiber).
 * RESIDUALS. every body is unwritten: 0x80478790-0x8047ABC4 (61 functions).
 * SHAPES. seams measured so far: 0x80478790 (gki_timers_init, the first function after GKI_delete_pool) and
 *   0x8047ABC4 (the first 4-aligned start, BTE/bd.c).
 */

#include "BTE/gki_time.h"
