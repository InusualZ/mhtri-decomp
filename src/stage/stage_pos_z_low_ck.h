/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w`'s `stage_pos_z_low_ck`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_STAGE_STAGE_POS_Z_LOW_CK_H
#define MHTRI_STAGE_STAGE_POS_Z_LOW_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0688 (stage/stg_w): whether the position's z is below the stage's limit. */
u32 stage_pos_z_low_ck(void* self /* untyped: byte range - the position whose z is tested */);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_POS_Z_LOW_CK_H */
