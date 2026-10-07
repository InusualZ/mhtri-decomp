/*
 * SEQ/seq.c - a MIDI sequencer: its init and quit, the MIDI event dispatch and the per-frame sequence runner.
 *
 * RANGE. .text 0x804DD350-0x804DD9C0 (5 functions, 0x670 B); .data 0x80629F18-0x80629F98; .sbss
 *    0x80795468-0x80795470; .sdata2 0x8079D330-0x8079D350.  Cut from the old SC block between
 *    `SC/SCProductInfo.c` (0x804DD350) and `SI/SIBios.c` (0x804DD9C0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the library name `SEQ`, the file name `seq.c` and the unit's role are GUESSES read off the bodies; none of
 *    the five rows is named in the map or the dump.
 * EVIDENCE. `.data` 0x80629F18 (0x80 B) is a MIDI message length table indexed by status byte, read by the event
 *    decode at 0x804DD350 (which writes events through 0x804DF1B0 of the synthesizer unit); `.sbss`
 *    0x80795468/0x8079546C is the init flag and the sequence list head read by 0x804DD430, 0x804DD450 and
 *    0x804DD460; `.sdata2` 0x8079D330..0x8079D350 are the tempo/tick constants of the per-frame runner; the
 *    callers are `sound/snd_stream_reloc.cpp` (init at 0x804DD430, per frame at 0x804DD460).
 * RESIDUALS. no bodies yet: all 5 functions are unwritten (largest fn_804DD460, 0x410 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
