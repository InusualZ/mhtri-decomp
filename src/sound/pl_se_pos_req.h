/* `pl_se_pos_req`, defined by `sound/fn_800D7F54.cpp` (leaf header, rule 2). */
#ifndef MHTRI_SOUND_PL_SE_POS_REQ_H
#define MHTRI_SOUND_PL_SE_POS_REQ_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Requests the owner's landing sound (25, or 45 where the ground test at `pos` says so) at `pos`. */
void pl_se_pos_req(struct _PLW* plw, nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_SE_POS_REQ_H */
