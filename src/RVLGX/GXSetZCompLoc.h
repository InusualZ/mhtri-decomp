/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `GXSetZCompLoc` (0x804B9FC0), owned by `RVLGX/GXPixel.c`.  Separate from
 * `RVLGX/GXPixel.h` so `main.cpp`, which still declares other pixel-engine entry points locally, can include it alone.
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
