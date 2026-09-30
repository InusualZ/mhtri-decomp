/* enemy/em040_ai.cpp - the em040 enemy's action band, `.text` 0x801B98C8..0x801BB758 (no bodies written yet).
 *
 * What it is.  One translation unit cut out of the old `enemy/fn_801B7020.cpp` range (2026-09-30 recut): the
 * enemy action/state machines between em036's static initializer and em006's first function.  Its extab
 * 0x8000F904..0x8000F9A4 and extabindex 0x8002B470..0x8002B560 are the entries of its own functions.
 *
 * Seam.  Left edge 0x801B98C8: `fn_801B985C`, the `.ctors` word 0x8056F34C, is em036's static initializer and
 * MWCC emits a TU's `__sinit` last; `fn_801B98C8` is an ordinary function (`get_move_work_adrs`/`get_move_work_max`
 * loop), and the 0.0 pool entry is repeated at `lbl_80798DA4` from there on (window 0x801B98C8..0x801B9968).
 * Right edge 0x801BB758: the 0.0 entry is repeated again at `lbl_80798E40`, first read by `fn_801BB758` alone (a
 * one-function window), and the strong `.sdata2` cut there agrees.
 *
 * Name.  GUESS from the data order: `em040_prog_tbl` (0x805B2808, 0x70 bytes) is the first object of this TU's
 * `.data` chunk (em036's chunk starts at `em036_prog_tbl` 0x805B2118, em006's at `em006_prog_tbl` 0x805B28D0),
 * the same shape the registered `enemy/em019_ai.cpp`/`em024_ai.cpp` names follow; no `__FILE__` string survives.
 *
 * What is unknown.  Every body (the range's inventory is `ledger.py unit enemy/em040_ai.cpp` and the map);
 * nothing is written, so the unit's `.text` is not measured.
 */

#include "types.h"
