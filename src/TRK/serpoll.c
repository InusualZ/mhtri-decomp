/*
 * TRK/serpoll.c - the MetroTRK serial poll layer: `TRKTestForPacket`, `TRKGetInput`, `TRKProcessInput`,
 *    `TRKInitializeSerialHandler` and its terminate twin.
 *
 * RANGE. .text 0x80469788..0x804698D0 (5 functions in the map, 0x148 B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `serpoll`).
 * EVIDENCE. dump names; call-graph closure (all callees are the buffer and UART layers); no data.
 * RESIDUALS. COARSE.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/serpoll.c`).
 */
