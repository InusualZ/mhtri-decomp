/*
 * Pl/bss_pool.cpp - the Pl band's shared `.bss` collision-work run, registered as a data-only unit.
 *
 * The unit owns `.bss` 0x806AB848-0x806AC8A8 (0x1060 B, 12 arrays: the per-chunk move-work table, the
 * hit-box and land tables, the per-slot collision-result arrays and the hit-id list).  Its source
 * **defines nothing**: the bytes are the original's, and a `NonMatching` unit contributes exactly
 * those bytes to the link (this object emits no data at all).  What the registration buys is that the
 * run has **one owner**: consumers include `Pl/bss_pool.h` (rule 2's home) instead of declaring the
 * arrays into their own file, which is what rule 12 fires on, and dtk no longer generates an anonymous
 * `auto_*_bss` unit over the same band.
 *
 * Range: `.bss` 0x806AB848-0x806AC8A8, 4-aligned on both ends.  Both edges are measured with
 * `tools/units/callers.py`: 0x806AB83C before the run is read only by `Pl/fn_80288CEC.cpp`, and
 * 0x806AC8A8 - the array `include/Pl/fn_80295EF4.h` types as `HitRegistry` - is the first address a
 * non-`Pl` unit reads (`menu/menu_item.cpp`), so the run stops there.  The array's other readers are
 * the `Pl` ground/hit collision band alone: `Pl/fn_8028F66C.cpp`, `Pl/fn_80295EF4.cpp`,
 * `Pl/fn_8025F088.cpp` and `Pl/pl_act.cpp`.
 *
 * The 12 map rows were `lbl_806ABxx`; they are named from the construction `fn_80297C30` performs with
 * `__construct_array` (element ctor and stride per array) and from the read sites - each name's
 * evidence is the comment beside it in `Pl/bss_pool.h` and the unit outbox.
 *
 * Naming: the file name is a GUESS - no `__FILE__` string and no runtime-dump name covers the range,
 * so it keeps the name `tools/units/dataclaim.py` derives for the pool (`Pl/bss_pool.cpp`); the arrays
 * no written body spells yet carry GUESS names (the header says which).
 *
 * Measured when the claim landed: the whole-project report is unchanged row for row and its totals
 * are identical except `total_units` (the run changes owner, it does not change size) while
 * `ninja build/RMHE08/ok` stays green and the DOL hash is unchanged.
 *
 * Residual: the lower half of the band, `.bss` 0x806AC8A8-0x806AD698 (~0xDD0 B, `HitRegistry` and the
 * menu item work tables), stays unowned - it is shared by `menu/menu_item`, `menu/fn_8031EA8C`,
 * `ai/fn_802D44F4`, `menu/menu_row` and `lobby/fn_802076D4`, so it needs a **joint** named owner whose
 * header those five modules declare into, and `lbl_806AC8A8` (Pl-typed - `Pl/fn_80295EF4.h` declares
 * `HitRegistry lbl_806AC8A8`) is that owner's first row, not this unit's.
 */

#include "types.h"
#include "Pl/bss_pool.h"
