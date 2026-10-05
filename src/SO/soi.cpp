/*
 * SO/soi.cpp - STUB (no bodies yet).
 *
 * `.text` 0x8051E864..0x8052A040.  Sections of the candidate unit: .text 0x8051E864..0x8052A040; .rodata 0x80574E10..0x80579450; .data 0x80631230..0x806498C8; .bss 0x80766C40..0x807901C0; .sdata 0x80794448..0x807944B0; .sbss 0x807958D8..0x80795A18; .sdata2 0x8079D528..0x8079D550.
 *
 * WHAT IT IS. the SO socket library (`SOiGetSysWork`, `SOClose`, `SOBind`, ...) plus VF, the DB interface, PMIC, KPR, HID and KBD up to the HOME-button run at 0x8052A040.
 *   merged candidate pieces (guess cuts the renderer does not emit): soinit, soi, vf, dbinterface, pmic, kpr, hid, kbd.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class inherited: 0x8051E864 follows the registered unit NWC24/nwc24_io.c with no function in between (<= 16 B pad); baseline order/coverage/text-cut PASS.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit SO/soi.cpp`), and the pass that writes the bodies defines them.
 */
