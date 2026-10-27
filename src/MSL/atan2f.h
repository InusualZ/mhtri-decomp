/* `atan2f`, defined by `MSL/mathf.cpp` (leaf header, rule 2: `MSL/mathf.h` cannot be included beside
 * `unsplit/unknown.h`, whose `sinf` spelling differs). */
#ifndef MHTRI_MSL_ATAN2F_H
#define MHTRI_MSL_ATAN2F_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the single-precision arc tangent of `y / x`. */
f32 atan2f(f32 y, f32 x);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MSL_ATAN2F_H */
