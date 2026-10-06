/*
 * WPAD/wpad.cpp - STUB (no bodies yet).
 *
 * `.text` 0x804E45B0..0x80500868.  Sections of the candidate unit: .text 0x804E45B0..0x80500868; .rodata 0x80573C40..0x80573CD8; .data 0x8062AB68..0x8062F030; .bss 0x8075B110..0x80760C48; .sdata 0x80794148..0x807941E0; .sbss 0x807955C8..0x80795758; .sdata2 0x8079D3C0..0x8079D480.
 *
 * WHAT IT IS. a merged block of TPL, USB (`IUSB_*`), VI, i2c, WENC, WPAD and WUD, ending in three nw4r::db console / ut
 *   functions (0x80500770-0x80500868) that only `nw4r/db_assert.cpp` and other units call; 301 functions.
 *   merged candidate pieces (guess cuts the renderer does not emit): TPL, usb, fn_804E5600, vi, i2c, vi3in1, WENC, wbc_dummy, wpad, wud, WUDHidHost.
 *
 * WHY IT SITS HERE. phase 1 grade strong, class anchor: TPLBind (the `TPL.c` __FILE__ anchor)/TPLGet.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit WPAD/wpad.cpp`), and the pass that writes the bodies defines them.
 */
