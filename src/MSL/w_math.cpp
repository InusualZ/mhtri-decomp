/*
 * MSL/w_math.cpp - the libm wrapper entries: six 4-byte tail thunks (`acos`, `asin`, `atan2`, `fmod`, `log10`,
 *    `pow`), the `sqrt` wrapper with the EDOM errno path, a bare return and the 4-byte `sqrt` thunk.
 *
 * RANGE. .text 0x80467F64..0x804681CC (9 functions in the map, 0x268 B); .sdata2 0x8079CF38..0x8079CF40.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MSL `w_*` wrappers merged).
 * EVIDENCE. each thunk is a single `b` to the matching fdlibm unit (e_acos, e_asin, e_atan2, e_fmod, e_log10,
 *    e_pow); the 0x248-byte function reads `errno`, `__float_nan` and `.sdata2` 0x8079CF38 (`1.0`).
 * RESIDUALS. COARSE: the thunks may be several files; no data separates them.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL/w_math.cpp`).
 */
