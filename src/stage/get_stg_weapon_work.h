/*
 * `get_stg_weapon_work` (0x802AE0C4), defined by `stage/stg_w.cpp` (leaf header, rule 2), and the placed-weapon record
 * its tables hold (`StageWeaponWork`, read field by field by `Pl/pl_act.cpp`'s `pl_stg_weapon_near_find`).
 */
#ifndef MHTRI_STAGE_GET_STG_WEAPON_WORK_H
#define MHTRI_STAGE_GET_STG_WEAPON_WORK_H

#include "types.h"
#include "ef.h" /* Vec, the engine vector */

/* One placed weapon of a map area (the stage's `.data` tables, 0x20 bytes apart).  size: 0x20 */
typedef struct StageWeaponWork {
    /* +0x00 */ s32 kind;       /* the weapon kind the finder returns; 2 is tested against a height band */
    /* +0x04 */ s32 word_0x04;  /* handed back through the finder's last out parameter */
    /* +0x08 */ Vec pos;        /* the engine-vector position (`vec_to_mh_vec3` converts it) */
    /* +0x14 */ f32 radius;     /* the xz reach */
    /* +0x18 */ f32 height;     /* the vertical reach */
    /* +0x1C */ u16 id;         /* handed back through the finder's id out parameter */
    /* +0x1E */ u8 pad_0x1E[0x2];
} StageWeaponWork;

#ifdef __cplusplus
StageWeaponWork* get_stg_weapon_work(struct _PLW* plw, long* count);
#endif

#endif /* MHTRI_STAGE_GET_STG_WEAPON_WORK_H */
