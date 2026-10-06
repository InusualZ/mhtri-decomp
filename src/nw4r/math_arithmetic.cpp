/*
 * nw4r/math_arithmetic.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80500CF8..0x80500E34.  Sections of the candidate unit: .text 0x80500CF8..0x80500E34; .data 0x8062F0B0..0x8062F9C0; .sdata2 0x8079D480..0x8079D4A0.
 *
 * WHAT IT IS. nw4r::math's table-driven exp and log (`detail::FExp`, `detail::FLog`: a sample table at .data
 *   0x8062F0B0 / 0x8062F1B8 interpolated by the fraction; both called from `g3d/g3d_anmchr.cpp`) and
 *   `nw4r::math::FrSqrt(float)`; 3 functions.  The left-edge evidence is in `nw4r/db_assert.cpp`'s header.
 *
 * NAMES. `detail::FExp`/`detail::FLog` are a GUESS from the nw4r source, read off the bodies (a 2^n exponent split
 *   for exp, the float's exponent field for log) and the log caller's NaN constant (.sdata2 0x80795D50, nw4r's
 *   inline `math::FLog` returns NaN for x <= 0).
 *
 * WHY IT SITS HERE. phase 1 grade medium, class file: FrSqrt__Q24nw4r4mathFf: nw4r::math arithmetic roster.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/math_arithmetic.cpp`), and the pass that writes the bodies defines them.
 */
