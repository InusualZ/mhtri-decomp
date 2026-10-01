/* sound/sound_job.cpp - the sound/effect job request: `.text` 0x800E3B3C..0x800E3CBC (the stage's effect size class, the job-slot fill
 * `sound_job_request` and the motion-record clear), `.sdata2` 0x8.
 *
 * Phase 4: the tail of the old `sound/fn_800DD1F0.cpp`; the reconciled candidate cuts it at 0x800E3B3C (its own extab group, and the
 * `.sdata2` pair the bodies read).  Name: `sound_job` after the map's `sound_job_request`; no `__FILE__` string names the TU.
 * The old unit's functions here are moved unchanged; only `fn_800E3C90` has a body.
 */

#include "types.h"
#include "nw4r/math.h"
#include "sound/se.h" /* the pool constants this unit reads (lbl_80796450/54) */

#pragma peephole off

/* One 0x34-byte motion record of the model's motion table. size: 0x34 */
struct MhMotionEntry {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 pad_0x0A[0x0E];
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u8 pad_0x1C[2];
    /* +0x1E */ u16 field_0x1E;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ f32 field_0x24;
    /* +0x28 */ f32 field_0x28;
    /* +0x2C */ u8 pad_0x2C[8];
};

/* Clear a motion record and seed its two rate constants. */
extern "C" void fn_800E3C90(MhMotionEntry* entry)
{
    entry->field_0x18 = 0;
    entry->field_0x20 = 0;
    entry->field_0x1E = 0;
    entry->field_0x08 = 0;
    entry->field_0x09 = 0;
    entry->field_0x24 = lbl_80796450;
    entry->field_0x28 = lbl_80796454;
}
