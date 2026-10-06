/* nw4r/math_triangular.h - the cross-unit declarations of `nw4r/math_triangular.cpp`: nw4r::math's table-driven
 *   trigonometry over the 256-steps-per-circle angle index. */
#ifndef MHTRI_NW4R_MATH_TRIANGULAR_H
#define MHTRI_NW4R_MATH_TRIANGULAR_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {

/* One sine/cosine sample and the step to the next. size: 0x10 */
struct SinCosSample {
    /* +0x0 */ f32 sin_val;
    /* +0x4 */ f32 cos_val;
    /* +0x8 */ f32 sin_delta;
    /* +0xC */ f32 cos_delta;
};

/* .rodata 0x80573CD8 - one sample per index step, plus the closing sample at index 256 (`ef/ef_util.cpp`
 * reads it directly). */
extern const SinCosSample sSinCosTbl[256 + 1];

/* 0x80500E34 - the sine of an angle index. */
f32 SinFIdx(f32 fidx);

/* 0x80500E9C - the cosine of an angle index. */
f32 CosFIdx(f32 fidx);

/* 0x80500F60 - the angle of (x, y) in index units, -128..128. */
f32 Atan2FIdx(f32 y, f32 x);

/* 0x80501108 - the angle of (x, y) as a 16-bit index, 0..65535 per circle. */
u16 Atan2Idx(f32 y, f32 x);

}  // namespace math
}  // namespace nw4r

extern "C" {
#endif

/* 0x80500EF4 - writes the sine and the cosine of an angle index (nw4r's `SinCosFIdx`, C name in the map). */
void math_sincos_idx(f32* s, f32* c, f32 fidx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW4R_MATH_TRIANGULAR_H */
