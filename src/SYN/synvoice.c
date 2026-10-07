/*
 * SYN/synvoice.c - a software synthesizer on top of AX, part three: the voice parameter functions and the tables they
 *    read.
 *
 * RANGE. .text 0x804E01E0-0x804E1120 (21 functions, 0xF40 B); .data 0x8062A2A0-0x8062AB20; .sdata2
 *    0x8079D380-0x8079D3A0.  Cut from the old SC block between `SYN/synenv.c` (0x804E01E0) and `THP/THPDec.c`
 *    (0x804E1120).  COARSE: the library's source files of this half are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the file name `synvoice.c` is a GUESS (the library name `SYN` is a GUESS too); no row is named in the map or
 *    the dump.
 * EVIDENCE. `.data` 0x8062A2A0..0x8062AB20 are its tables (0x100, 0x200, 0x190, 0x3F0 B), `.sdata2`
 *    0x8079D380..0x8079D3A0 its float constants (the first is the int-to-double constant again, see
 *    `SYN/syn.c`); the last pool value `0x4330000000000000` (0x8079D390) is held again at 0x8079D3A0 by
 *    `THP/THPDec.c`.
 * RESIDUALS. no bodies yet: all 21 functions are unwritten (largest fn_804E09F0, 0x214 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
