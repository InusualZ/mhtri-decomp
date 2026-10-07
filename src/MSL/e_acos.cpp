/*
 * MSL/e_acos.cpp - the fdlibm arc-cosine kernel (`__ieee754_acos`), one function.
 *
 * RANGE. .text 0x80463FFC..0x804642C8 (1 functions in the map, 0x2CC B); .sdata2 0x8079CA30..0x8079CAB8.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS: acos rather than another by the constants and the first thunk of `MSL/w_math.cpp` that
 *    targets it.
 * EVIDENCE. its `.sdata2` pool 0x8079CA30..0x8079CAB8 carries pi, pi/2 and the pS/qS coefficients; `0.0` is pooled
 *    at 0x8079CA28 (wide printf unit) and its `2.0` again at 0x8079CAB0 / 0x8079CB30 (the arc-sine unit that
 *    follows), so it is a TU of its own; the thunk at 0x80467F64 tail-calls it.
 * RESIDUALS. no body written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL/e_acos.cpp`).
 */
