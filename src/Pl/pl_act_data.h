/*
 * Pl/pl_act_data.h - the `.sdata2` constant run 0x80799F98..0x80799FDC (17 named words) of `Pl/pl_act_step.cpp`'s
 *   pool, declared, never defined (playbook 29): 0x80799F98..0x80799FBC is loaded by the act state-machine band,
 *   0x80799FC0..0x80799FD8 by the per-frame control cluster, 0x80799F98 by both.
 * The file name is a GUESS from its readers' role.  The values are read from the target's `.sdata2` and the names
 *   follow `Pl/pl_frame_data.h`'s scheme (`pl_frame_window_<n>` for an integral frame window, `pl_float_<value>`
 *   otherwise); each comment's role is the load site that identifies it.
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
extern const f32 pl_float_10000;	/* 0x80799FBC: 10000 - range test on `vec3_dist_sq`'s distance
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
