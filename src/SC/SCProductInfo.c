/*
 * SC/SCProductInfo.c - the SC product information: `__SCF1` (the product info file reader) and the product area, code
 *    and serial getters.
 *
 * RANGE. .text 0x804DD010-0x804DD350 (5 functions, 0x340 B); .data 0x80629EB8-0x80629F18; .sdata
 *    0x80794110-0x80794138; .sbss 0x80795460-0x80795468.  Cut from the old SC block between `SC/SCApi.c`
 *    (0x804DD010) and `SEQ/seq.c` (0x804DD350).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SCGetProductArea`, `SCGetProductCode`, `SCGetProductSN` and `__SCF1` are the map's names; 0x804DD2C0 is
 *    unnamed; the file name `SCProductInfo.c` is a GUESS.
 * EVIDENCE. `__SCF1` is called only by the four product getters; `.sdata` 0x80794110..0x80794138 are their short
 *    string literals; `.data` 0x80629EB8 (the area table `JPN`, `USA`, `EUR`, ...) and 0x80629F00 (the region
 *    table `JP`, `US`, `EU`, `KR`, `CN`) are read by `SCGetProductArea` and the 0x804DD2C0 region lookup;
 *    `.sbss` 0x80795460 (8 B) by `SCGetProductCode`.
 * RESIDUALS. no bodies yet: all 5 functions are unwritten (largest __SCF1, 0x170 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
