/*
 * Pl/pl_act_data.cpp - the Pl band's second shared `.sdata2` constant run, registered as a
 * data-only unit.
 *
 * The unit owns `.sdata2` 0x80799F98-0x80799FDC (68 B, 17 words: the act frame windows, distance
 * thresholds and blend factors the player act state machine and its per-frame control cluster
 * load).  Its source **defines nothing**: the bytes are the original's, and a `NonMatching` unit
 * contributes exactly those bytes to the link (this object emits no data at all).  What the
 * registration buys is that the run has **one owner**: consumers include `Pl/pl_act_data.h` (rule
 * 2's home) instead of declaring the words into their own file, which is what rule 12 fires on, and
 * dtk no longer generates an anonymous `auto_*_sdata2` unit over the same band.
 *
 * The run is the MWLD *merge* of two objects' own pools - one address is read by both of them and
 * the rest split into two disjoint ordered referrer runs (`tools/units/callers.py 0x80799F98`
 * answers `Pl/fn_80258FCC.cpp` and `Pl/fn_8025F088.cpp`) - so no single consumer can emit it alone
 * and the owner is the model for it (playbook 23/53 route 2).
 *
 * Range: `.sdata2` 0x80799F98-0x80799FDC.  Both edges are measured: the word before the run
 * (0x80799F94, `pl_frame_window_50`) is loaded only by `Pl/pl_act_step.cpp`, and the word after it
 * (0x80799FDC) only by `Pl/fn_80262940.cpp`, whose own pool starts there.  The run is the head of
 * dtk's tail bulk unit `auto_11_80799F98_sdata2` (0x80799F98-0x8079B740, 6056 B), which the claim
 * truncates at 0x80799FDC.  4-aligned on both ends.
 *
 * The 17 map rows were `lbl_80799Fxx`; they are named from their values in `Pl/pl_frame_data.h`'s
 * scheme for the same pool (integral frame window -> `pl_frame_window_<n>`, anything else ->
 * `pl_float_<value>`), and each role recorded in the header is the load site that identifies it.
 *
 * Naming: the file name is a GUESS (no `__FILE__` string and no runtime-dump name covers the range)
 * - see the header, which states the evidence and why a context name was preferred over
 * `dataclaim.py`'s `Pl/sdata2_pool.cpp` default.
 *
 * Measured when the claim landed: the whole-project report's totals are unchanged except
 * `total_units` (the run changes owner, it does not change size) and `ninja build/RMHE08/ok` stays
 * green.  Residual: the `.bss` move-work table at 0x806AB848 is **not** this run's - the target
 * object carries no `.bss` - so it was declared in `Pl/fn_8025F088.h` until the run got its own
 * data-only owner, `Pl/bss_pool.cpp` (its header is `Pl/bss_pool.h`).
 */

#include "types.h"
#include "Pl/pl_act_data.h"
