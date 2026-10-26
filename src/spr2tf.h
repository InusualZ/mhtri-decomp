/*
 * The 2D sprite record `drawSpr2TF` (userdata_item.cpp) queues: shared by its owner and its callers
 * (`fn_80059550.cpp` builds them), so it lives in one header (docs/plan.md 6.5 rule 1).
 */
#ifndef MHTRI_SPR2TF_H
#define MHTRI_SPR2TF_H

#include "types.h"

/* The screen rectangle, a depth (`drawSpr2TF` scales it by 1/1024), the vertex colour and the texel rectangle
 * (`r`/`g` the first corner's s/t, `b`/`a` the second's; `drawSpr2TF` divides them by the texture's size less one).
 * size: 0x18 */
typedef struct fltSpr2TF {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 w;
    /* +0x06 */ s16 h;
    /* +0x08 */ s16 angle; /* the depth, scaled by 1/1024 into the queued sprite */
    /* +0x0A */ s16 pad_0x0A;
    /* +0x0C */ u32 color;
    /* +0x10 */ s16 r;
    /* +0x12 */ s16 g;
    /* +0x14 */ s16 b;
    /* +0x16 */ s16 a;
} fltSpr2TF;

#endif /* MHTRI_SPR2TF_H */
