/* Declarations for the two `.sdata` style-table directories at 0x807922B0/0x807922B8, which no registered unit's
 * range covers (docs/plan.md 6.5 rule 2: the band is their home until a unit claims them).  `lobby/lb_equip_page.cpp`'s
 * `pl_rig_hair_style_get`/`pl_rig_face_style_get` index them by the actor's hair-style set (`_PLW::se_name_set`). */
#ifndef MHTRI_UNSPLIT_PL_RIG_STYLE_H
#define MHTRI_UNSPLIT_PL_RIG_STYLE_H

#include "types.h"

struct PlRigHairStyle;
struct PlRigFaceStyle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x807922B0 - per hair-style set, its table of 8-byte hair-style rows. */
extern struct PlRigHairStyle* pl_rig_hair_style_tbls[2];
/* 0x807922B8 - per hair-style set, its table of 2-byte face-style rows. */
extern struct PlRigFaceStyle* pl_rig_face_style_tbls[2];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_PL_RIG_STYLE_H */
