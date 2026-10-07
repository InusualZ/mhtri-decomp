/*
 * THP/THPDec.c - the THP movie decoder: the Huffman and IDCT decode of a frame and `THPInit`.
 *
 * RANGE. .text 0x804E1120-0x804E45B0 (16 functions, 0x3490 B); .rodata 0x80573BB0-0x80573C40; .data
 *    0x8062AB20-0x8062AB68; .bss 0x8075AFC0-0x8075B110; .sdata 0x80794140-0x80794148; .sbss
 *    0x807954A0-0x807955C8; .sdata2 0x8079D3A0-0x8079D3C0.  Cut from the old SC block at 0x804E1120; the right
 *    edge is `TPLBind` (0x804E45B0, `TPL/tpl.cpp`).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. only the library name `THP` is known (the build string); the file name `THPDec.c` is a GUESS and every row
 *    is unnamed.
 * EVIDENCE. `.data` 0x8062AB20 is the build string `<< RVL_SDK - THP release build ... >>` read through `.sdata`
 *    0x80794140 by the last function (0x804E4510, `THPInit`); `.rodata` 0x80573BB0 (0x50 B) and 0x80573C00
 *    (0x40 B) are tables read by the decoder functions (0x804E1650, 0x804E3150, 0x804E37D0, 0x804E3E70); the
 *    three 0x698 B routines use paired-single instructions and the decoder uses the locked cache
 *    (`LCStoreData`, `LCQueueWait`, `PPCMfhid2`); `.bss` 0x8075AFC0 .. 0x8075B110 and `.sbss`
 *    0x807954A0..0x807955C8 are read here only; the callers are `menu/movie.cpp` (0x804E1120, 0x804E4510).
 * RESIDUALS. no bodies yet: all 16 functions are unwritten (largest fn_804E37D0, 0x698 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
