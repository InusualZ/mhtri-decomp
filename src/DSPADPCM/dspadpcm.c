/*
 * DSPADPCM/dspadpcm.c - the DSP-ADPCM sample encoder: the encoded size formula, the encoder entry and its coefficient
 *    search.
 *
 * RANGE. .text 0x804719A0-0x80474CB0 (11 functions, 0x3310 B); .sdata2 0x8079CF68-0x8079CFF0.  Cut from the old
 *    ARC/AX block at 0x804719A0 (after `__AXGetCurrentProfile`); the right edge is the start of
 *    `AX/AXFXReverbHi.c` (0x80474CB0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `getBytesForAdpcmSamples` and `encodeAdpcmSamples` are the map's names (GUESS: the sample-size formula); the
 *    library name `DSPADPCM` and the file name `dspadpcm.c` are GUESSES; nine rows are unnamed.
 * EVIDENCE. no AX state is touched: the eleven functions call each other and `DCFlushRange` only; `.sdata2`
 *    0x8079CF68..0x8079CFF0 (doubles) is read by the encoder functions and by nothing before 0x804719A0; the
 *    one outside caller is the network voice path (`Network/NetworkWiiMediator.cpp`).
 * RESIDUALS. no bodies yet: all 11 functions are unwritten (largest fn_80472390, 0xCD8 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
