/* `math_sin_rad`, defined by `ef/ef_particle.cpp` (leaf header, rule 2): the weak copy of nw4r's SinRad. */
#ifndef MHTRI_EF_MATH_SIN_RAD_H
#define MHTRI_EF_MATH_SIN_RAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB658 (0xC): the sine of `x` radians. */
f32 math_sin_rad(f32 x);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MATH_SIN_RAD_H */
