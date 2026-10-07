/*
 * AI_SDK/ai.c - the Revolution SDK AI (audio interface) library: DMA init/start/stop, the DMA byte counter,
 *    `AIInit` and its interrupt handler with the callback stack switch.
 *
 * RANGE. .text 0x8046D420..0x8046D9F0 (12 functions in the map, 0x5D0 B); .data 0x8060F820..0x8060F868; .sdata
 *    0x80793D00..0x80793D08; .sbss 0x80794E88..0x80794EC8.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. directory `AI_SDK` because `src/ai/` already holds the game's monster AI and the paths collide on a case-
 *    insensitive checkout; file name `ai` is the library's.
 * EVIDENCE. its text is a 16-aligned run inside the 4-byte-packed TRK band (0x8046D420 starts after
 *    `GetUseSerialIO`); `.data` 0x8060F820 is the build string `<< RVL_SDK - AI ...` read through `.sdata`
 *    0x80793D00 by `AIInit`; `.sbss` 0x80794E88..0x80794EC8 are read only by `AIInit`, `__AIDHandler`,
 *    `__AICallbackStackSwitch` and the first function.
 * RESIDUALS. no bodies written; the lib block is `OS` (cflags_os), flags unmeasured; the 16-byte function
 *    alignment of the run is not applied (cflags_os packs on 4).
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit AI_SDK/ai.c`).
 */
