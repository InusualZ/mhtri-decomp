/* `se_req_pos_impact` (0x800DC53C), defined by `sound/fn_800D7F54.cpp` (leaf header, rule 2). */
#ifndef MHTRI_SOUND_SE_REQ_POS_IMPACT_H
#define MHTRI_SOUND_SE_REQ_POS_IMPACT_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DC53C - plays sound 232 (`flag` 0) or 233 at `pos`. */
void se_req_pos_impact(u8 flag, nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_REQ_POS_IMPACT_H */
