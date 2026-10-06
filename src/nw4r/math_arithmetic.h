/* nw4r/math_arithmetic.h - the cross-unit declarations of `nw4r/math_arithmetic.cpp`: nw4r::math's table-driven
 *   exp and log, the reciprocal square root, and the inline fast casts and absolute value the math units share.
 *   C++ callers name the owner, and the front-end emits the map's manglings. */
#ifndef MHTRI_NW4R_MATH_ARITHMETIC_H
#define MHTRI_NW4R_MATH_ARITHMETIC_H

#include "types.h"
#include "OS/OSFastCast.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {
namespace detail {

/* 0x80500CF8 - e^x from a 2^n exponent split and a sampled fraction table (.data 0x8062F0B0). */
f32 FExp(f32 x);

/* 0x80500D84 - ln x from the float's exponent field and a sampled mantissa table (.data 0x8062F1B8). */
f32 FLog(f32 x);

}  // namespace detail

/* 0x80500E10 - 1/sqrt(x): the hardware estimate refined by one Newton step. */
f32 FrSqrt(f32 x);

/* The absolute value through the `fabs` instruction. */
inline f32 FAbs(register f32 x) {
    register f32 ret;
    asm { fabs ret, x }
    return ret;
}

/* The bit pattern of a float and back. */
inline u32 F32AsU32(f32 x) {
    return *reinterpret_cast<u32*>(&x);
}

inline f32 U32AsF32(u32 x) {
    return *reinterpret_cast<f32*>(&x);
}

/* The unbiased exponent of a float. */
inline s32 FGetExpPart(f32 f) {
    s32 s = static_cast<s32>((F32AsU32(f) >> 23) & 0xFF);
    return s - 127;
}

/* The mantissa of a float, scaled into [1, 2). */
inline f32 FGetMantPart(f32 f) {
    return U32AsF32((F32AsU32(f) & 0x807FFFFF) | 0x3F800000);
}

/* Float to unsigned 16-bit and back through the OS fast-cast GQR. */
inline u16 F32ToU16(f32 x) {
    u16 rval;
    OSf32tou16(&x, &rval);
    return rval;
}

inline f32 U16ToF32(u16 x) {
    f32 rval;
    OSu16tof32(&x, &rval);
    return rval;
}

/* Float to signed 16-bit and back through the OS fast-cast GQR. */
inline s16 F32ToS16(f32 x) {
    s16 rval;
    OSf32tos16(&x, &rval);
    return rval;
}

inline f32 S16ToF32(s16 x) {
    f32 rval;
    OSs16tof32(&x, &rval);
    return rval;
}

}  // namespace math
}  // namespace nw4r
#endif

#endif /* MHTRI_NW4R_MATH_ARITHMETIC_H */
