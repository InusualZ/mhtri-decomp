/* mh3_pad/vec3_assign.h - the leaf header of `vec3_assign`, defined in `mh3_pad.cpp` (0x80041E70, the kept copy).
 * `nw4r/math.h` includes it after `_VEC3`, so `VEC3::operator=` can call it while the other units see only this
 * declaration. */
#ifndef MHTRI_MH3_PAD_VEC3_ASSIGN_H
#define MHTRI_MH3_PAD_VEC3_ASSIGN_H

namespace nw4r {
namespace math {
struct _VEC3;
}  // namespace math
}  // namespace nw4r

/* 0x80041E70 (0x1C): copies a vector's three components (retail's out-of-line copy of `_VEC3`'s implicit
 * assignment, the call `VEC3::operator=` makes). */
extern "C" void vec3_assign(nw4r::math::_VEC3* dst, const nw4r::math::_VEC3* src) __attribute__((never_inline));

#endif
