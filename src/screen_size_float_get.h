/* Leaf header (docs/plan.md 6.5 rule 2): `screen_size_float_get` (0x8004030C), defined by `src/main.cpp`; declared for
 * `src/draw_shape_arm.cpp`.  C linkage (the map row is a plain name).  GUESS name (name registry). */
#ifndef MHTRI_SCREEN_SIZE_FLOAT_GET_H
#define MHTRI_SCREEN_SIZE_FLOAT_GET_H

#include "types.h"

struct _MH_VEC2;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004030C (0x54): writes the screen size, read as signed 16-bit, as two floats. */
void screen_size_float_get(struct _MH_VEC2* v);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCREEN_SIZE_FLOAT_GET_H */
