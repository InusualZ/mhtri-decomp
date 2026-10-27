/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s `se_req_frame_set_sp` (C++ scope: `se_req_frame_set_sp__FP5_se_wlllll`). */
#ifndef MHTRI_SOUND_SE_REQ_FRAME_SET_SP_H
#define MHTRI_SOUND_SE_REQ_FRAME_SET_SP_H

#include "types.h"
struct _se_w; /* sound/fn_800D7F54.cpp */

/* 0x800DA230 (0x1F8): requests a framed sound effect on the work `work`. */
void se_req_frame_set_sp(struct _se_w* work, s32 a, s32 param, s32 d, s32 e, s32 f);

#endif /* MHTRI_SOUND_SE_REQ_FRAME_SET_SP_H */
