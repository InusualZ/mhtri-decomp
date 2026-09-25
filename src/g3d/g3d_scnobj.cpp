/*
 * nw4r g3d: g3d_scnobj.cpp - the ScnObj base object and its ScnLeaf/ScnGroup subclasses.
 * `.text` 0x800813B8-0x800827E4 (36 functions, 5164 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `8007C540` - this
 * unit is one of the four original translation units that proposal's range turned out to span.
 *
 * Name - evidence class 1, a `__FILE__` string: the unit's `.data` fragment (0x8058F3D8-0x8058F530)
 * opens on `g3d_scnobj.cpp` (lbl_8058F3D8, referenced by dtor_800813B8, fn_80081A30, fn_8008209C,
 * fn_80082524 and dtor_800825CC), and the same fragment holds the `!GetParent()` assert, the
 * `NW4R:Pointer must not be NULL (pBuf)` panic and two switch tables (jumptable_8058F40C,
 * jumptable_8058F434).  Module `g3d`, language C++ (`.cpp` name, C++ `Panic` call sites).
 *
 * Seams.  Left: 0x800813B8 (tudiscover weak `codegen fingerprint change` cut, the extent of this
 * unit's proven match set - 29 functions, "certainly one TU", anchored by `g3d_scnobj.cpp` - and the
 * `.data` fragment boundary).  Right: 0x800827E4, where g3d/g3d_scnroot.cpp begins
 * (`codegen fingerprint change` cut, `g3d_scnroot.cpp` starts the next `.data` fragment at
 * 0x8058F530, the match set is anchored in that unit).
 *
 * The 0x80082668-0x800827E4 tail (7 functions) is allocated to this unit: fn_800826AC, fn_80082714 and
 * fn_8008277C are the name-record readers for ScnObj, ScnLeaf and ScnGroup
 * (`return *fn_800638B8(&local, lbl_8056F6A0/6B0/6C0)`), the three classes this file defines, in the
 * order the records appear in `.rodata` - and only the TU defining them can register them.
 *
 * Sections claimed: `.text` 0x800813B8-0x800827E4, `extab` 0x800087D4-0x800088F4,
 * `extabindex` 0x80021150-0x800212C4.  The `.data` fragment 0x8058F3D8-0x8058F530 is not claimed
 * (docs/plan.md 8.4); it is recorded for the data pass.
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX`/`dtor_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` and by reading every `.text` entry in
 * 0x800813B8..0x800827E4 out of config/RMHE08/symbols.txt).  `dtor_` stems are the map's own
 * placeholders for the two deleting destructors.
 *
 * Reconstruction status: registration only - the bodies are the next pass's work.  Per-unit scores
 * are in the outbox.
 */

#include "types.h"
