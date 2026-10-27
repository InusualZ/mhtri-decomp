/* Leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s `rmode_fb_width_get`, for `font/flfnt.cpp`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_RMODE_FB_WIDTH_GET_H
#define MHTRI_RMODE_FB_WIDTH_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80040274 (0xC): returns the render mode's framebuffer width (`Rmode->fbWidth`). */
u16 rmode_fb_width_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RMODE_FB_WIDTH_GET_H */
