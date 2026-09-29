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
 * (0x80799E00 `pl_float_zero` .. 0x80799F94 `pl_frame_window_50`; the next named row is the
 * unrelated `lbl_80799F98`).  4-aligned on both ends.
 *
 * Measured when the claim moved here from `Pl/pl_act_step.cpp`: the whole-project report is unchanged
 * row for row (the 408 B of `total_data` simply changes unit) and `ninja build/RMHE08/ok` stays green.
 */

#include "types.h"
#include "Pl/pl_frame_data.h"
