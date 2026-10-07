/*
 * SI/SIBios.c - the SI (serial interface) library: controller polling, transfers, the type probe and the sampling
 *    rate.
 *
 * RANGE. .text 0x804DD9C0-0x804DF0A0 (21 functions, 0x16E0 B); .data 0x80629F98-0x8062A0A0; .bss
 *    0x80757608-0x80757828; .sdata 0x80794138-0x80794140; .sbss 0x80795470-0x80795488.  Cut from the old SC block
 *    between `SEQ/seq.c` (0x804DD9C0) and `SYN/syn.c` (0x804DF0A0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SIBusy`, `SIInit`, `SITransfer`, `SIGetType`, `SISetSamplingRate`, ... are the map's names; five rows are
 *    unnamed; the file name `SIBios.c` is a GUESS.
 * EVIDENCE. `.data` 0x80629F98 is the build string `<< RVL_SDK - SI release build ... (0x4302_145) >>` read through
 *    `.sdata` 0x80794138 by `SIInit`; `Si`, `Type`, the 0x30 B `XYNTSC` table and the 0x30 B table after it
 *    with the `SISetSamplingRate: unknown TV format` string follow (0x8062A008, 0x8062A038); `.bss` 0x80757608
 *    (`Packet`) .. 0x807577E8 and `.sbss` 0x80795470..0x80795488 are read here only; `PAD/pad.c` calls
 *    `SIBusy`.
 * RESIDUALS. no bodies yet: all 21 functions are unwritten (largest SIInterruptHandler, 0x3EC B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
