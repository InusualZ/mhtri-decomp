/*
 * TRK/CircleBuffer.c - the MetroTRK circular byte queue: initialise, write bytes, read bytes and their lock helpers.
 *
 * RANGE. .text 0x804685F0..0x80468868 (7 functions in the map, 0x278 B).
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name is the dump's prefix.
 * EVIDENCE. dump names `CircleBufferInitialize` / `WriteBytes` / `ReadBytes`; the three unnamed helpers before
 *    them are called only by these.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/CircleBuffer.c`).
 */
