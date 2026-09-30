/*
 * g3d/g3d_xsi.cpp - the nw4r g3d XSI-scene helpers: `.text` 0x800D6C00..0x800D77B0 (15 functions, 2992 B), extab 0x8000AC0C..0x8000AC54,
 * extabindex 0x80024258..0x800242C4, `.rodata` 0x8056F830..0x8056F868, `.data` 0x80595790..0x80595840 (the `g3d_xsi.cpp` file string
 * and three pointer-error / flag assert strings read by `fn_800D74E8`), `.sdata2` pool 0x807963C8..0x807963D0 (shared by every function of the run).
 * Registered 2026-09-30, carved out of `nw_resource.cpp`'s tail.
 *
 * Seams: the right edge is `g3d/fn_800D77B0.cpp` (`tudiscover at 0x800D74E8`: g3d_xsi.cpp -> g3d_basic.cpp, `.sdata2` 0x807963CC ->
 * 0x807963D0).  The left edge is the end of the big function `fn_800D5F9C`: the run's 15 functions are one TU by the shared pool, and
 * the previous function's strings sit in the preceding TU's inline tail.  Name from the `__FILE__` string (class 1).
 *
 * Unwritten: every function; `.rodata`/`.data` are claimed and not emitted yet.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit g3d/g3d_xsi.cpp`.
 */

#include "types.h"
