/* font/flfnt_frame_reset.h - leaf header (docs/plan.md 6.5 rule 2): `font/flfnt.cpp`'s font frame reset, size reads and debug-draw init, for `menu/movie.cpp`.  `flfnt_frame_reset` is a GUESS name (the registry carries it). */
#ifndef MHTRI_FONT_FLFNT_FRAME_RESET_H
#define MHTRI_FONT_FLFNT_FRAME_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005B278 - lays the font memory out again and clears its buffers. */
void flfnt_frame_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FONT_FLFNT_FRAME_RESET_H */
