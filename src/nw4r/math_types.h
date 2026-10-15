/*
 * nw4r/math_types.h - declarations of the matrix helpers `nw4r/math_types.cpp` defines.
 */
#ifndef NW4R_MATH_TYPES_H
#define NW4R_MATH_TYPES_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void mtx34_rotate_vec3(nw4r::math::VEC3* out, const nw4r::math::MTX34* mtx, const nw4r::math::VEC3* v);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace math {

MTX33* MTX33Identity(MTX33* pOut);
MTX33* MTX34ToMTX33(MTX33* pOut, const MTX34* pM);
MTX34* MTX34Zero(MTX34* pOut);
MTX34* MTX34Scale(MTX34* pOut, const MTX34* pM, const VEC3* pS);
MTX34* MTX34Trans(MTX34* pOut, const MTX34* pM, const VEC3* pT);
MTX34* MTX34RotAxisFIdx(MTX34* pOut, const VEC3* pAxis, f32 fIdx);
MTX34* MTX34RotXYZFIdx(MTX34* pOut, f32 fx, f32 fy, f32 fz);
MTX34* MTX33ToMTX34(MTX34* pOut, const MTX33* pM);
MTX44* MTX44Identity(MTX44* pOut);
MTX44* MTX44Copy(MTX44* pOut, const MTX44* pM);

}  // namespace math
}  // namespace nw4r
#endif

#endif
