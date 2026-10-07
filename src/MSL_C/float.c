/*
 * MSL_C/float.c - the C library float constants (data only): `__float_nan` (0x80793CF0), `__float_huge`
 *    (0x80793CF4) and `__double_huge` (0x80793CF8).
 *
 * RANGE. .sdata 0x80793CF0..0x80793D00.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `float.c`); the three symbols are GUESS names (the MSL names of the quiet NaN, the
 *    float infinity and the double infinity; the dump has none).
 * EVIDENCE. `.sdata` 0x80793CF0..0x80793D00 is read by the scanner, the number conversion, the string-to-float
 *    unit and the fdlibm units (sharing across units means a data-only TU); it follows the string unit's statics
 *    and precedes the AI library's `.sdata` in link order. The words are 0x7FFFFFFF, 0x7F800000 and the pair
 *    0x7FF00000 / 0; the fdlibm callers load them through `lis`/`lfs` (an array of unknown extent), so the
 *    symbols are word arrays.
 * RESIDUALS. none measured.
 * SHAPES. word arrays, read by consumers through a cast to the float or double they stand for.
 */
#include "MSL_C/float.h"

u32 __float_nan[1] = {0x7FFFFFFF};
u32 __float_huge[1] = {0x7F800000};
u32 __double_huge[2] = {0x7FF00000, 0};
