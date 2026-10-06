/* enemy/em016_prog_types.h - the `EmActWork` view of the enemy work record and its records, shared by
 * `enemy/em016_prog.cpp` and `enemy/em018_prog.cpp`. */
#ifndef MHTRI_ENEMY_EM016_PROG_TYPES_H
#define MHTRI_ENEMY_EM016_PROG_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

/* The mangled callees take `_ENEMY_WORK*`/`_CP_VECTOR*`, so the tags are forward-declared and the calls cast
 * this view onto them (rule 9). */
struct _ENEMY_WORK;

/* ------------------------------------------------------------------------------------------------ */
/* this range's view of the records it reads                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* Three 32-bit fixed-point angles (`_CP_VECTOR`'s layout, under this unit's own name).
 * size: 0x0C */
struct EmRotVec {
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
};

/* The aim/animation target record at `EmActWork::aim_0x328`.
 * size: 0x3C */
struct EmAimRec {
    /* +0x00 */ u32 angle_0x00;    /* the lead angle: +-0x222 per frame */
    /* +0x04 */ EmRotVec rot_0x04; /* step +-0x1C7; z is armed to -1 by the initialiser */
    /* +0x10 */ EmRotVec rot_0x10; /* step +-0x1C7 */
    /* +0x1C */ VEC3 vec_0x1C;     /* the vector `fn_8008E8D0` is handed */
    /* +0x28 */ f32 value_0x28;    /* armed with `value_0x2C` before bit 0 of `flags_0x35` */
    /* +0x2C */ f32 value_0x2C;
    /* +0x30 */ u8 flag_0x30;
    /* +0x31 */ u8 flag_0x31;
    /* +0x32 */ u8 flag_0x32;
    /* +0x33 */ u8 flag_0x33;      /* nonzero: `fn_801923CC` reports */
    /* +0x34 */ u8 flag_0x34;
    /* +0x35 */ u8 flags_0x35;     /* bit 0x01 `fn_80192618`, 0x02 `fn_801923E0`, 0x08 `fn_80192410` */
    /* +0x36 */ s16 value_0x36;
    /* +0x38 */ s16 value_0x38;
    /* +0x3A */ s16 value_0x3A;
};

/* One of the three per-area clusters at `EmActWork::clusters_0x590` (`enemy/fn_80138074.c` addresses the same
 * array as `i * 0x90 + 0x590`); only the liveness byte and the position are read.  size: 0x90 */
struct EmCluster {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 live_0x03;     /* `fn_80191E30` needs >= 1 */
    /* +0x04 */ u8 unused_0x04[0x20];
    /* +0x24 */ VEC3 pos_0x24;    /* the work's +0x5B4 */
    /* +0x30 */ u8 unused_0x30[0x60];
};

/* The enemy work record, as this range's accesses measure it.
 * size: 0xB18 */
struct EmActWork {
    /* +0x000 */ u8 active;       /* `fn_80192448` skips an empty record */
    /* +0x001 */ u8 unused_0x001[0x003 - 0x001];
    /* +0x003 */ u8 team;         /* the dispatch key of `fn_80191598` (16/17/21) and `fn_80192448`
                                   * (0x12) */
    /* +0x004 */ u8 unused_0x004[0x00A - 0x004];
    /* +0x00A */ u8 field_0x00A;  /* 1 selects the second entry block of the dispatch */
    /* +0x00B */ u8 unused_0x00B[0x188 - 0x00B];
    /* +0x188 */ VEC3 pos_0x188;
    /* +0x194 */ u8 unused_0x194[0x1E0 - 0x194];
    /* +0x1E0 */ u8 field_0x1E0;  /* the key `stage_map_kind_get` (map lookup) is called with */
    /* +0x1E1 */ u8 act_id;       /* the action id every dispatch in this range switches on */
    /* +0x1E2 */ u8 state_0x1E2;
    /* +0x1E3 */ u8 unused_0x1E3[0x328 - 0x1E3];
    /* +0x328 */ EmAimRec aim_0x328;
    /* +0x364 */ u8 unused_0x364[0x590 - 0x364];
    /* +0x590 */ EmCluster clusters_0x590[3];
    /* +0x740 */ u8 unused_0x740[0x9F6 - 0x740];
    /* +0x9F6 */ u8 state_0x9F6;
    /* +0x9F7 */ u8 unused_0x9F7[0xB18 - 0x9F7];
};
#endif /* MHTRI_ENEMY_EM016_PROG_TYPES_H */
