/* enemy/em040_ai.cpp - the em040 enemy's action/state machines, between em036's static initializer and em006's first
 *   function.
 * RANGE. .text 0x801B98C8-0x801BB758 (31 functions); .data 0x805B2804-0x805B28D0, .sdata2 0x80798D88-0x80798E40,
 *   extab, extabindex.
 * SEAM. Left edge: `fn_801B985C` (`.ctors` 0x8056F34C) is `enemy/fn_801B7020.cpp`'s static initializer, which MWCC
 *   emits last in a TU, and the 0.0 pool entry repeats at `lbl_80798DA4` from `fn_801B98C8` on (window
 *   0x801B98C8-0x801B9968).  Right edge: the 0.0 entry repeats again at `lbl_80798E40`, first read by `fn_801BB758`
 *   alone, and the strong `.sdata2` cut there agrees.
 * NAMES. `em040_ai` is a GUESS from the data order: `em040_prog_tbl` (0x805B2808) is the first object of the unit's
 *   `.data` (em036's starts at `em036_prog_tbl` 0x805B2118, em006's at `em006_prog_tbl` 0x805B28D0), the shape
 *   `enemy/em019_ai.cpp` and `enemy/em024_ai.cpp` follow.
 * RESIDUALS. 31 rows unwritten: 0x801B98C8-0x801BB758 (the whole range).
 *   flipcheck: the object emits none of the claimed sections (`.data`, `.sdata2`, `.text`, extab, extabindex).
 */

#include "types.h"
