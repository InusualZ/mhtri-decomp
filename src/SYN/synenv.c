/*
 * SYN/synenv.c - a software synthesizer on top of AX, part two: two exponential curve functions (they call `pow`)
 *    over the 0x200 B table.
 *
 * RANGE. .text 0x804DFC50-0x804E01E0 (2 functions, 0x590 B); .data 0x8062A0A0-0x8062A2A0; .sdata2
 *    0x8079D360-0x8079D380.  Cut from the old SC block between `SYN/syn.c` (0x804DFC50) and `SYN/synvoice.c`
 *    (0x804E01E0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the file name `synenv.c` is a GUESS from the `pow` curves (the library name `SYN` is a GUESS too); no row is
 *    named in the map or the dump.
 * EVIDENCE. the `0x4330000080000000` constant is held at 0x8079D378 here and again at 0x8079D380 for the next unit;
 *    `.data` 0x8062A0A0 (0x200 B) is read by 0x804DFC50, 0x804DFF10 and, as an extern, by the next unit;
 *    `.sdata2` 0x8079D360..0x8079D380 are the constants of the two functions. The cut after 0x804DFF10 is
 *    placed at the first function that reads a table only the next unit's data order can own (0x804E01E0 reads
 *    0x8062A5A0, which follows 0x8062A2A0 in `.data`; the pool window is 0x804E01E0..0x804E0470).
 * RESIDUALS. no bodies yet: all 2 functions are unwritten (largest fn_804DFF10, 0x2C4 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
