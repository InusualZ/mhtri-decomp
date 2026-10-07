/*
 * KPR/kpr.c - the KPR library: the keyboard character queue and its character conversion lookups.
 *
 * RANGE. .text 0x80526C90-0x805272B0 (9 functions, 0x620 B); .rodata 0x80579088-0x80579450; .data
 *    0x806492D8-0x80649348; .sdata 0x80794498-0x807944A0; .sbss 0x807959F0-0x80795A00.  Cut from the old VF block
 *    between `PMIC/pmic.c` (0x80526C90) and `HID/hid.c` (0x805272B0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `KPRLookAhead` is a GUESS (the old VF unit's); the library name `KPR` is the build string's and the file
 *    name `kpr.c` a GUESS; eight rows are unnamed.
 * EVIDENCE. `.data` 0x806492D8 is the build string `<< RVL_SDK - KPR ... >>` read through `.sdata` 0x80794498 by
 *    0x80526CA0; `.rodata` 0x80579088 (0x288 B) and 0x80579310/0x80579350 are the character conversion tables
 *    of 0x80527140 and 0x80526FD0; `.sbss` 0x807959F0..0x80795A00 is read here only; `KPRLookAhead` is called
 *    by `homebutton/hbm_text_panel.cpp`.
 * RESIDUALS. no bodies yet: all 9 functions are unwritten (largest fn_80526FD0, 0x170 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
