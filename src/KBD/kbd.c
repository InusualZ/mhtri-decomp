/*
 * KBD/kbd.c - the KBD library: the keyboard driver (`KBDResetChannel`, key processing, key maps, LED requests and
 *    channel records).
 *
 * RANGE. .text 0x80528890-0x8052A040 (28 functions, 0x17B0 B); .data 0x806493A0-0x806498C8; .bss
 *    0x8078FF40-0x807901C0; .sdata 0x807944A8-0x807944B0; .sbss 0x80795A08-0x80795A18.  Cut from the old VF block
 *    at 0x80528890; the right edge is `homebutton/fn_8052A040.cpp`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `kbdProcKey`, `KBDResetChannel`, `kbdInitMap`, `kbdInitMapIntl`, `kbd_free_led`, `kbd_alloc_led`,
 *    `kbd_initialize_led_module` and `kbd_init_keyboard` are the map's names; `KBDSetLedsAsync` and
 *    `KBDSetChannelValue` are GUESSES; the file name `kbd.c` is a GUESS; 18 rows are unnamed.
 * EVIDENCE. `.data` 0x806493A0 is the build string `<< RVL_SDK - KBD ... >>` read through `.sdata` 0x807944A8 by
 *    0x80529820, followed by the key map tables (0x806493E8, 0x806497D0) and two jump tables; `.sbss`
 *    0x80795A08..0x80795A18 and `.bss` 0x8078FF40 (0x200 B), 0x80790140 (0x80 B) are read here only;
 *    `KBDSetLedsAsync` and `KBDSetChannelValue` are called by `homebutton/tiHKBManager.cpp`.
 * RESIDUALS. no bodies yet: all 28 functions are unwritten (largest fn_80528F80, 0x310 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
