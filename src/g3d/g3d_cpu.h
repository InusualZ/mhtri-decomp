/* g3d/g3d_cpu.h - the 32-byte block copy and zero fill `g3d/g3d_cpu.cpp` owns (nw4r::g3d::detail). */
#ifndef MHTRI_G3D_G3D_CPU_H
#define MHTRI_G3D_G3D_CPU_H

#include "types.h"

namespace nw4r {
namespace g3d {
namespace detail {

/* 0x8009A748 - copies `size` bytes (a multiple of 32) from `pSrc` to `pDst`. */
void Copy32ByteBlocks(void* pDst, const void* pSrc, u32 size); /* untyped: byte range */
/* 0x8009A910 - fills `size` bytes (a multiple of 32) at `pDst` with 0.0f. */
void ZeroMemory32ByteBlocks(void* pDst, u32 size); /* untyped: byte range */

}  // namespace detail
}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_G3D_G3D_CPU_H */
