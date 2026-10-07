/*
 * SYN/syn.c - a software synthesizer on top of AX, part one: voice init, the per-frame run, MIDI input and the event
 *    handlers.
 *
 * RANGE. .text 0x804DF0A0-0x804DFC50 (9 functions, 0xBB0 B); .bss 0x80757828-0x8075AFC0; .sbss 0x80795488-0x807954A0;
 *    .sdata2 0x8079D350-0x8079D360.  Cut from the old SC block between `SI/SIBios.c` (0x804DF0A0) and
 *    `SYN/synenv.c` (0x804DFC50).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the library name `SYN` and the file name `syn.c` are GUESSES read off the bodies; no row is named in the map
 *    or the dump.
 * EVIDENCE. `.bss` 0x80757828 (0x3798 B of 0x94 B records, initialised by the loop at 0x804DF0A0 that also calls
 *    `AXIsInit` and the AX voice count) and `.sbss` 0x80795488..0x807954A0 (read by the third unit too) are
 *    defined here; `.sdata2` 0x8079D350 and 0x8079D358 are the constants of 0x804DFA60, the second the
 *    `0x4330000080000000` int-to-double constant, held again at 0x8079D378 (`SYN/synenv.c`) and 0x8079D380
 *    (`SYN/synvoice.c`): one TU pools a value once, so the three are separate TUs (the cut is the first
 *    function with a pool read, 0x804DFC50; 0x804DFBE0 is the other candidate); the callers are
 *    `sound/snd_stream_reloc.cpp` (init at 0x804DF0A0, per frame at 0x804DF140) and the sequencer
 *    (0x804DF1B0).
 * RESIDUALS. no bodies yet: all 9 functions are unwritten (largest fn_804DF200, 0x414 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
