/*
 * BTE/gki_buffer.cpp - STUB (no bodies yet).
 *
 * `.text` 0x804772F0..0x804AFED0.  Sections of the candidate unit: .text 0x804772F0..0x804AFED0; .rodata 0x80573530..0x80573A00; .data 0x80612D18..0x8061A4F0; .bss 0x8070CDE0..0x80746B60; .sdata 0x80793D28..0x80793E30; .sbss 0x80795008..0x80795198; .sdata2 0x8079D090..0x8079D0F0; .sbss2 0x8079D7E0..0x8079D7F0.
 *
 * WHAT IT IS. the Broadcom BTE Bluetooth stack (gki, hcisu, bta_dm/hh, btm, btu, gap, hci, hidd/hidh, l2c, port/rfc, sdp) plus the DB, DSP, DVD, ENC, ESP and EUART libraries that follow it up to 0x804AFED0; 895 functions of 183 KB.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_804772F0, gki_buffer, gki_time, gki_common, hcisu_h2, uusb, bte_main, btu_task, bta_sys_conn, bta_sys_main, ptim, utl, bta_dm_act, bta_dm_api, ....
 *
 * WHY IT SITS HERE. phase 1 grade medium, class inherited: 0x804772F0 follows the registered unit OS/PPCArch.c with no function in between (<= 16 B pad); baseline order/coverage/text-cut PASS.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit BTE/gki_buffer.cpp`), and the pass that writes the bodies defines them.
 */
