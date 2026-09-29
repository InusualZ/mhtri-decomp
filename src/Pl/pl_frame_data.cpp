/*
 * Pl/pl_frame_data.cpp - the Pl band's shared frame-window/float pool, registered as a data-only unit.
 *
 * The unit owns `.sdata2` 0x80799E00-0x80799F98 (408 B: 78 named words plus the unnamed interior),
 * the run every `_PLW` unit in the Pl band reads for its frame windows and its 0.0/1.0-style
 * constants.  Its source **defines nothing**: the bytes are the original's, and a `NonMatching` unit
 * contributes exactly those bytes to the link (this object emits no data at all).  What the
 * registration buys is that the run has **one owner**: consumers include `Pl/pl_frame_data.h` (rule
 * 2's home) instead of declaring the words into their own file, which is what rule 12 fires on, and
 * dtk no longer generates an anonymous `auto_*_sdata2` unit over the same band.
 *
 * The run is the MWLD *merge* of several Pl objects' own pools - one address is read by several
 * `lfs ...@sda21` sites whose owners are different TUs (`callers.py 0x80799E40` answers 7 sites in 5
 * functions across 3 objects, and 0x80799E00 is read by `Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`
 * and this unit) - so no single consumer can emit it alone and the owner is the model for it.
 *
 * Range: `.sdata2` 0x80799E00-0x80799F98, the exact extent of the map's `pl_*` `.sdata2` rows
 * (0x80799E00 `pl_float_zero` .. 0x80799F94 `pl_frame_window_50`; the row that follows is
 * `pl_float_neg250`, owned by `Pl/pl_act_data.cpp`).  4-aligned on both ends.
 *
 * Measured when the claim moved here from `Pl/pl_act_step.cpp`: the whole-project report is unchanged
 * row for row (the 408 B of `total_data` simply changes unit) and `ninja build/RMHE08/ok` stays green.
 *
 * Consumer conversion (this pass): the four units that still declared the run's words themselves -
 * `Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`, `Pl/fn_80258FCC.cpp`, `Pl/fn_8025F088.cpp` - now include
 * this owner's header, and the duplicates in `include/unsplit/Pl.h` (a band header may not declare a
 * symbol a registered unit owns, rule 2) and in `include/Pl/fn_8025F088.h` are gone.  Eight of the
 * words they spelled had only `lbl_80799Exx` rows and were named from their values, the same scheme as
 * the rest of the run (`lbl_80799E48` -> `pl_float_neg8`, `lbl_80799E84` -> `pl_frame_window_80`, ...,
 * `lbl_80799EC4` -> `pl_float_0_66`); the map rows and every reference moved in the same change.
 *
 * Residual: the `.sdata2` run that follows this one, 0x80799F98-0x80799FDC (17 words), is **not**
 * ours: it was claimed by `Pl/pl_act_data.cpp`, which owns it and declares its words in
 * `Pl/pl_act_data.h`.  The band's shared `.bss` collision-work run (`pl_move_work` and the collision
 * arrays) is a different pool: it now has its own data-only owner, `Pl/bss_pool.cpp`
 * (`Pl/bss_pool.h`).
 */

#include "types.h"
#include "Pl/pl_frame_data.h"
