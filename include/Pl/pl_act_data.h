/*
 * Declarations owned by `Pl/pl_act_data.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring a pool word itself.
 *
 * The unit is **data-only**: it owns the Pl band's second shared `.sdata2` constant run,
 * `0x80799F98..0x80799FDC` (68 B, 17 named words), and its source defines nothing.  For a
 * `NonMatching` unit the original bytes stay in the binary, so the DOL is untouched while the run
 * gains a single owner - which is what stops rule 12 firing for the run's consumers and what stops
 * dtk creating an anonymous `auto_*_sdata2` unit over the same bytes.  The run is the MWLD *merge*
 * of two objects' own pools, so no single consumer can emit it: the referrer runs are disjoint and
 * ordered - `0x80799F98..0x80799FBC` is loaded by `Pl/fn_80258FCC.cpp`, `0x80799FC0..0x80799FD8` by
 * `Pl/fn_8025F088.cpp`, with `0x80799F98` read by both.
 *
 * The extent is measured, not guessed: the word before the run (`0x80799F94`) is loaded only by
 * `Pl/pl_act_step.cpp` and the word after it (`0x80799FDC`) only by `Pl/fn_80262940.cpp`, so the run
 * is the head of dtk's tail bulk unit `auto_11_80799F98_sdata2` (0x80799F98-0x8079B740) and the next
 * owner's pool starts exactly at 0x80799FDC.
 *
 * The unit name is a **GUESS**: no `__FILE__` string and no runtime-dump name covers the range, so
 * the file is named for its readers' role (the player act state-machine band and the per-frame
 * control cluster) and for what the words are used as - act frame windows, distance thresholds and
 * blend factors.  `tools/units/dataclaim.py`'s default for a pool like this is `Pl/sdata2_pool.cpp`;
 * a context name is preferred over the section name (docs/plan.md 6.5 rule 7 is read as "not a
 * generated name", but `sdata2_pool` says only which section the bytes are in).
 *
 * The values in the comments are read out of the target object's `.sdata2` and the names follow
 * `Pl/pl_frame_data.h`'s scheme for the same pool: an integral frame window is
 * `pl_frame_window_<n>`, any other constant is `pl_float_<value>`.  Each role is the load site that
 * identifies it, so a later pass can refine the name where the value alone does not decide it.
 *
 * This unit owns this run only.  `lbl_806AB848` (`.bss` 0x806AB848, 0x420 B, declared in
 * `Pl/fn_8025F088.h`) is a different pool shared by 11 units, and the target object of
 * `Pl/fn_8025F088.cpp` carries no `.bss` at all, so it is not this unit's.
 */
#ifndef MHTRI_PL_ACT_DATA_H
#define MHTRI_PL_ACT_DATA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const f32 pl_float_neg250;	/* 0x80799F98: -250 - floor of the `_PLW`+0x6C decay
                                         * (`fn_80259C38`, `fn_8025FA00`) */
extern const f32 pl_float_neg2000;	/* 0x80799F9C: -2000 - stored into `_PLW`+0x80 by `fn_80259EF8` */

extern const f32 pl_frame_window_164;	/* 0x80799FA0: 164 - `Pl_frame_check` window (`fn_8025B0F8`) */
extern const f32 pl_frame_window_68;	/* 0x80799FA4: 68 - `Pl_frame_check` window (`fn_8025C09C`) */
extern const f32 pl_frame_window_168;	/* 0x80799FA8: 168 - `Pl_frame_check` window (`fn_8025C09C`) */
extern const f32 pl_frame_window_150;	/* 0x80799FAC: 150 - `Pl_frame_check` window (`fn_8025C27C`) */
extern const f32 pl_frame_window_114;	/* 0x80799FB0: 114 - `Pl_frame_check` window (`fn_8025CCB8`,
                                         * `fn_8025D250`) */
extern const f32 pl_float_neg70;	/* 0x80799FB4: -70 - compared against `_PLW`+0x110 (`fn_8025E448`) */
extern const f32 pl_float_240;	/* 0x80799FB8: 240 - added to `pl_frame_window_60` for the
                                 * `fn_80277C94` offset (`fn_8025E448`) */
extern const f32 pl_float_10000;	/* 0x80799FBC: 10000 - range test on `fn_80050EAC`'s distance
                                     * (`fn_8025EFF4`) */

extern const f32 pl_float_0_3;	/* 0x80799FC0: 0.3 - blend factor of the `fn_800524C0` call (`fn_8025F088`) */
extern const f32 pl_float_110;	/* 0x80799FC4: 110 - added to `_PLW`+0x60 before the +0x4C compare
                                 * (`fn_8025F088`) */
extern const f32 pl_float_0_6;	/* 0x80799FC8: 0.6 - second `fn_800524C0` blend factor (`fn_8025F088`) */
extern const f32 pl_float_0_01;	/* 0x80799FCC: 0.01 - scale on the u32->f32 conversion product
                                 * (`fn_802602A0`) */
extern const f32 pl_float_2250000;	/* 0x80799FD0: 2250000 - initial nearest-distance sentinel
                                     * (`fn_802607C4`) */
extern const f32 pl_float_1000;	/* 0x80799FD4: 1000 - second argument of the `fn_802D7804` call
                                 * (`fn_80261770`) */
extern const f32 pl_float_300;	/* 0x80799FD8: 300 - distance test on `fn_80050EF4`'s result
                                 * (`fn_80262688`) */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ACT_DATA_H */
