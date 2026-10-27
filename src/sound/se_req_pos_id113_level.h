/* sound/se_req_pos_id113_level.h - leaf header: `se_req_pos_id113_level` (0x800DC560), owned by `sound/fn_800D7F54.cpp`. C linkage. */
#ifndef MHTRI_SOUND_SE_REQ_POS_ID113_LEVEL_H
#define MHTRI_SOUND_SE_REQ_POS_ID113_LEVEL_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Requests sound effect 113, 114 or 115 (by `level`, 0..2) at `pos`, or 213..215 by the position's zone test. */
void se_req_pos_id113_level(VEC3* pos, s32 level);
#ifdef __cplusplus
}
#endif

#endif
