/* Rmode.h - leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s render mode, for `menu/movie.cpp`. */
#ifndef MHTRI_RMODE_H
#define MHTRI_RMODE_H

#include "types.h"

struct GXRenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794768 (.sbss) - the render mode the video is set up with. */
extern struct GXRenderModeObj* Rmode;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RMODE_H */
