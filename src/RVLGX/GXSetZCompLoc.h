/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the Revolution SDK's `GXSetZCompLoc` (0x804B9FC0, `GXPixel.c`: after
 * `GXSetZMode`, before `GXSetPixelFmt`), whose map address falls in `RVLGX/GXTexture_tail.cpp`'s registered range.
 * Split from `RVLGX/GXSetTevOrder.h` so `main.cpp`, which still declares other GX entry points locally, can include it
 * alone.
 */
#ifndef MHTRI_RVLGX_GXSETZCOMPLOC_H
#define MHTRI_RVLGX_GXSETZCOMPLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetZCompLoc(u8 before_tex);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXSETZCOMPLOC_H */
