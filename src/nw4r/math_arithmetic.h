/* nw4r/math_arithmetic.h - the cross-unit declarations of `nw4r/math_arithmetic.cpp`: nw4r::math's table-driven
 *   exp and log.  C++ callers name the owner, and the front-end emits the map's manglings. */
#ifndef MHTRI_NW4R_MATH_ARITHMETIC_H
#define MHTRI_NW4R_MATH_ARITHMETIC_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {
namespace detail {

/* 0x80500CF8 - e^x from a 2^n exponent split and a sampled fraction table (.data 0x8062F0B0). */
f32 FExp(f32 x);

/* 0x80500D84 - ln x from the float's exponent field and a sampled mantissa table (.data 0x8062F1B8). */
f32 FLog(f32 x);

}  // namespace detail
}  // namespace math
}  // namespace nw4r
#endif

#endif /* MHTRI_NW4R_MATH_ARITHMETIC_H */
