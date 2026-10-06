/*
 * g3d/g3d_xsi.cpp - nw4r g3d XSI-scene helpers.
 * RANGE. .text 0x800D6C00-0x800D77B0 (15 functions); extab, extabindex, .rodata 0x8056F830-0x8056F868, .data
 *   0x80595790-0x80595840 ("g3d_xsi.cpp" and the three assert strings fn_800D74E8 reads), .sdata2
 *   0x807963C8-0x807963D0 (shared by the whole run).  The left edge is the end of fn_800D5F9C in `nw_resource.cpp`'s
 *   band; the right edge is `g3d/fn_800D77B0.cpp` (tudiscover at 0x800D74E8: `.sdata2` 0x807963CC -> 0x807963D0).
 * NAMES. Map stems; the TU name is the `__FILE__` string.
 * RESIDUALS. All 15 functions unwritten (objdiff scores them zero); flipcheck: the object emits no section.
 */

#include "types.h"
