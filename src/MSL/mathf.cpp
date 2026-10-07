/*
 * MSL/mathf.cpp - the single-precision libm wrappers: `acosf`, `asinf`, `atan2f`, `cosf`, `fabsf`, `fmodf`,
 *    `tanf`, `sqrtf` and siblings, each widening to the double entry point, plus `scalbn`.
 *
 * RANGE. .text 0x80463DE4..0x80463FFC (14 functions in the map, 0x218 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS.
 * EVIDENCE. name run (`atan2f`, `cosf`, `fabsf`, `fmodf`, `tanf`); each 0x24-byte body is a call to one of the
 *    4-byte tail thunks in `MSL/w_math.cpp`; no data.
 * RESIDUALS. COARSE: `scalbn` (0x40) may be a separate file.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL/mathf.cpp`).
 */
