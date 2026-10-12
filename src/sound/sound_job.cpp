/* sound/sound_job.cpp - the sound/effect job request: `.text` 0x800E3B3C..0x800E3CBC (the stage's effect size class, the job-slot fill
 * `sound_job_request` and the motion-record clear), `.sdata2` 0x8.
 *
 * Phase 4: the tail of the old `sound/fn_800DD1F0.cpp`; the reconciled candidate cuts it at 0x800E3B3C (its own extab group, and the
 * `.sdata2` pair the bodies read).  Name: `sound_job` after the map's `sound_job_request`; no `__FILE__` string names the TU.
 * The old unit's functions here are moved unchanged; only `prim_rec_init` has a body.
 * NAMES. `split_screen_size_class`, `prim_rec_alloc` and `prim_rec_init` are GUESS names (from the bodies).
 */

#include "types.h"
#include "nw4r/math.h"
#include "sound/se.h" /* the pool constants this unit reads (lbl_80796450/54) */
#include "sound/fn_800E3CBC.h" /* PrimRec (rule 1) */
#include "sound/sound_job.h"

#pragma peephole off

/* Clear a record's state, handle, flags and callback and seed its two cull bounds. */
extern "C" void prim_rec_init(PrimRec* rec)
{
    rec->handle = 0;
    rec->callback = 0;
    rec->flags = 0;
    rec->state = 0;
    rec->started = 0;
    rec->limit_lo = lbl_80796450;
    rec->limit_hi = lbl_80796454;
}
