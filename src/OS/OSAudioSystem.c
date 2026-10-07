/*
 * OS/OSAudioSystem.c - the OS audio-system bring-up: AI clock init, audio system init and stop.
 * RANGE. .text 0x804CC130-0x804CC5F0 (3 functions); .data 0x8061C0D8-0x8061C158.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: `DSPInitCode` (.data 0x8061C0D8, 0x80 B) is read only by
 *    `__OSInitAudioSystem`; the three functions call the time base and the AI registers.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
