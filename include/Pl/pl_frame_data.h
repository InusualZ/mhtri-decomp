/*
 * Declarations owned by `Pl/pl_frame_data.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the pool word itself.
 *
 * The unit is **data-only**: it owns the Pl band's shared frame-window/float pool, the `.sdata2`
 * run `0x80799E00..0x80799F98` (408 B, 78 named words plus the unnamed interior), and its source
 * defines nothing.  For a `NonMatching` unit the original bytes stay in the binary, so the DOL is
 * untouched while the run gains a single owner - which is what stops rule 12 firing for every one of
 * the ~20 `_PLW` units that read these words (`Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`,
 * `Pl/fn_80258FCC.cpp`, `Pl/fn_8025F088.cpp`, `Pl/pl_act_step.cpp`, ...), and what stops dtk
 * creating an anonymous `auto_*_sdata2` unit for the same band.  The run is the MWLD *merge* of those
 * units' own pools (one address is read by several objects), so no single consumer can emit it; the
 * owner is the model for "the pool is one run, and it has one owner" (playbook 23/53 route 2).
 *
 * The values in the comments are read out of `orig/RMHE08/sys/main.dol` at each address, and the names
 * are derived from them in the band's scheme: an integral frame count is `pl_frame_window_<n>`, any
 * other constant is `pl_float_<value>` (`pl_float_neg8` for -8, `pl_float_0_66` for 0.66).  The words the
 * consumers above actually spell are named that way here; the run's remaining interior words keep their
 * `lbl_` rows and are named as the bodies that read them are written.
 *
 * The unit owns this run only.  The `.sdata2` run that follows it, 0x80799F98-0x80799FDC (17 words, read
 * by `Pl/fn_8025F088.cpp` and `Pl/fn_80258FCC.cpp`), has its own owner now - `Pl/pl_act_data.cpp`,
 * whose header `Pl/pl_act_data.h` declares those words.
 */
#ifndef MHTRI_PL_FRAME_DATA_H
#define MHTRI_PL_FRAME_DATA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const f32 pl_float_zero;	/* 0x80799E00: 0 */

extern const f32 pl_frame_window_1;	/* 0x80799E08: 1 */
extern const f32 pl_frame_window_180;	/* 0x80799E0C: 180 */

extern const f32 pl_frame_window_10;	/* 0x80799E18: 10 */
extern const f32 pl_frame_window_40;	/* 0x80799E1C: 40 */
extern const f32 pl_frame_window_3;	/* 0x80799E20: 3 */
extern const f32 pl_frame_window_90;	/* 0x80799E24: 90 */

extern const f32 pl_frame_window_2;	/* 0x80799E2C: 2 */
extern const f32 pl_float_0_8;	/* 0x80799E30: 0.8 */
extern const f32 pl_float_1_25;	/* 0x80799E34: 1.25 */
extern const f32 pl_frame_window_30;	/* 0x80799E38: 30 */

extern const f32 pl_frame_window_44;	/* 0x80799E40: 44 */
extern const f32 pl_frame_window_28;	/* 0x80799E44: 28 */
extern const f32 pl_float_neg8;	/* 0x80799E48: -8 */

extern const f32 pl_frame_window_20;	/* 0x80799E4C: 20 */
extern const f32 pl_float_neg1;	/* 0x80799E50: -1 */

extern const f32 pl_frame_window_8;	/* 0x80799E54: 8 */
extern const f32 pl_float_neg10;	/* 0x80799E58: -10 */

extern const f32 pl_frame_window_18;	/* 0x80799E64: 18 */
extern const f32 pl_float_neg20;	/* 0x80799E68: -20 */

extern const f32 pl_float_6_28319;	/* 0x80799E70: 6.28319 */
extern const f32 pl_float_0_03125;	/* 0x80799E74: 0.03125 */
extern const f32 pl_frame_window_360;	/* 0x80799E78: 360 */
extern const f32 pl_float_neg4;	/* 0x80799E7C: -4 */
extern const f32 pl_frame_window_100;	/* 0x80799E80: 100 */
extern const f32 pl_frame_window_80;	/* 0x80799E84: 80 */
extern const f64 pl_double_u32_to_f32_magic;	/* 0x80799E88: the 2^52 double MWCC's u32->f32
                                                 * conversion loads (8 B, 8-aligned) */

extern const f32 pl_float_1_1;	/* 0x80799E94: 1.1 */
extern const f32 pl_float_1_4;	/* 0x80799E98: 1.4 */
extern const f32 pl_frame_window_200;	/* 0x80799E9C: 200 */

extern const f32 pl_frame_window_60;	/* 0x80799EA8: 60 */
extern const f32 pl_frame_window_208;	/* 0x80799EAC: 208 */
extern const f32 pl_float_1_7;	/* 0x80799EB0: 1.7 */
extern const f32 pl_float_1_8;	/* 0x80799EB4: 1.8 */

extern const f32 pl_float_1_5;	/* 0x80799EC0: 1.5 */
extern const f32 pl_float_0_66;	/* 0x80799EC4: 0.66 */

extern const f32 pl_frame_window_96;	/* 0x80799ECC: 96 */
extern const f32 pl_frame_window_142;	/* 0x80799ED0: 142 */
extern const f32 pl_frame_window_190;	/* 0x80799ED4: 190 */
extern const f32 pl_frame_window_246;	/* 0x80799ED8: 246 */
extern const f32 pl_frame_window_238;	/* 0x80799EDC: 238 */
extern const f32 pl_frame_window_260;	/* 0x80799EE0: 260 */
extern const f32 pl_frame_window_16;	/* 0x80799EE4: 16 */
extern const f32 pl_frame_window_104;	/* 0x80799EE8: 104 */
extern const f32 pl_frame_window_118;	/* 0x80799EEC: 118 */
extern const f32 pl_frame_window_120;	/* 0x80799EF0: 120 */
extern const f32 pl_frame_window_46;	/* 0x80799EF4: 46 */
extern const f32 pl_float_0_95;	/* 0x80799EF8: 0.95 */
extern const f32 pl_float_0_9;	/* 0x80799EFC: 0.9 */
extern const f32 pl_frame_window_85;	/* 0x80799F00: 85 */
extern const f32 pl_float_neg60;	/* 0x80799F04: -60 */
extern const f32 pl_frame_window_5;	/* 0x80799F08: 5 */
extern const f32 pl_float_1_15;	/* 0x80799F0C: 1.15 */
extern const f32 pl_float_1_05;	/* 0x80799F10: 1.05 */
extern const f32 pl_float_10_5;	/* 0x80799F14: 10.5 */
extern const f32 pl_float_0_5;	/* 0x80799F18: 0.5 */
extern const f32 pl_frame_window_56;	/* 0x80799F1C: 56 */
extern const f32 pl_float_1_6;	/* 0x80799F20: 1.6 */
extern const f32 pl_float_neg0_72727;	/* 0x80799F24: -0.727273 */
extern const f32 pl_frame_window_195;	/* 0x80799F28: 195 */
extern const f32 pl_float_neg30;	/* 0x80799F2C: -30 */
extern const f32 pl_float_neg18;	/* 0x80799F30: -18 */
extern const f32 pl_frame_window_15;	/* 0x80799F34: 15 */
extern const f32 pl_float_neg15;	/* 0x80799F38: -15 */
extern const f32 pl_frame_window_42;	/* 0x80799F3C: 42 */
extern const f32 pl_frame_window_34;	/* 0x80799F40: 34 */
extern const f32 pl_float_neg22;	/* 0x80799F44: -22 */
extern const f32 pl_frame_window_94;	/* 0x80799F48: 94 */
extern const f32 pl_float_0_75;	/* 0x80799F4C: 0.75 */
extern const f32 pl_frame_window_84;	/* 0x80799F50: 84 */
extern const f32 pl_frame_window_4;	/* 0x80799F54: 4 */
extern const f32 pl_float_neg22_34;	/* 0x80799F58: -22.34 */
extern const f32 pl_float_neg0_33333;	/* 0x80799F5C: -0.333333 */
extern const f32 pl_float_neg19_04;	/* 0x80799F60: -19.04 */
extern const f32 pl_float_neg0_36;	/* 0x80799F64: -0.36 */
extern const f32 pl_frame_window_23;	/* 0x80799F68: 23 */
extern const f32 pl_float_neg0_25;	/* 0x80799F6C: -0.25 */
extern const f32 pl_frame_window_63;	/* 0x80799F70: 63 */
extern const f32 pl_frame_window_58;	/* 0x80799F74: 58 */
extern const f32 pl_frame_window_7;	/* 0x80799F78: 7 */
extern const f32 pl_float_neg7;	/* 0x80799F7C: -7 */
extern const f32 pl_float_1_2;	/* 0x80799F80: 1.2 */
extern const f32 pl_frame_window_64;	/* 0x80799F84: 64 */
extern const f32 pl_frame_window_88;	/* 0x80799F88: 88 */
extern const f32 pl_frame_window_170;	/* 0x80799F8C: 170 */
extern const f32 pl_frame_window_274;	/* 0x80799F90: 274 */
extern const f32 pl_frame_window_50;	/* 0x80799F94: 50 */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FRAME_DATA_H */
