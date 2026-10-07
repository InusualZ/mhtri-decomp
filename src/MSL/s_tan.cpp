/*
 * MSL/s_tan.cpp - the fdlibm tangent (`tan`), one function.
 *
 * RANGE. .text 0x80467EEC..0x80467F64 (1 functions in the map, 0x78 B); .sdata2 0x8079CF30..0x8079CF38.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name is the fdlibm name (`s_tan`) for the call pair.
 * EVIDENCE. calls `__ieee754_rem_pio2` and `__kernel_tan`; `.sdata2` 0x8079CF30 holds its `0.0` while the sine
 *    unit before it pools `0.0` at 0x8079CF28 (a value is pooled once per TU).
 * RESIDUALS. no body written; the lib block is `OS` (cflags_os) because it is cut from the old TRK unit, flags
 *    unmeasured.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL/s_tan.cpp`).
 */
