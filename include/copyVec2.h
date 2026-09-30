/* Leaf header: `copyVec2` (0x800403AC), the two-float copy `src/main.cpp` defines (it keeps its own
 * `_MH_VEC2` spelling of the signature; consumers see the float pair). */
#ifndef MHTRI_COPYVEC2_H
#define MHTRI_COPYVEC2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the two floats at `src` to `dst`. */
void copyVec2(f32* dst, const f32* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_COPYVEC2_H */
