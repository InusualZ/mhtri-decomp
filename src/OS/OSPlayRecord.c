/*
 * OS/OSPlayRecord.c - the OS play record: the record callback, its `b` tail-call wrapper and
 *    `__OSStartPlayRecord`/`__OSStopPlayRecord`.
 * RANGE. .text 0x804D5E00-0x804D6520 (4 functions); .data 0x806295E0-0x80629628; .bss 0x8074E160-0x8074E360; .sdata
 *    0x80793FC8-0x80793FD0; .sbss 0x807953D0-0x807953F0.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: the play-record path string and the callback jump table (.data 0x806295E0..0x80629628), the
 *    0x200-byte buffer (.bss 0x8074E160) and the `PlayRecord*` words are read only here; the 12-byte wrapper
 *    `fn_804D5E00` (0x804D5E00) tail-branches to the `scope:local` `PlayRecordCallback`, which pins the left edge
 *    two words before the callback (a local referenced from another unit is renamed by dtk, measured).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
