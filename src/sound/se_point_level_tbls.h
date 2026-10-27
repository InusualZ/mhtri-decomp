/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s point-SE level tables (registry name; GUESS). */
#ifndef MHTRI_SOUND_SE_POINT_LEVEL_TBLS_H
#define MHTRI_SOUND_SE_POINT_LEVEL_TBLS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .data 0x80597238 - ten pointers to 8-step level tables, indexed by a point zone's table byte. */
extern u8* se_point_level_tbls[10];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_POINT_LEVEL_TBLS_H */
