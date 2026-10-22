/* g3d/g3d_scninline.h - the declarations `g3d/g3d_scninline.cpp` owns (C linkage keeps the map's stems; the placement
 *   `operator new` is the global C++ one). */
#ifndef MHTRI_G3D_G3D_SCNINLINE_H
#define MHTRI_G3D_G3D_SCNINLINE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007BB8C/0x8007B72C - the type-name store copies the ScnLeaf and ScnGroup run-time type members call (and
 * `g3d/g3d_state.cpp`'s ScnRoot ones): each stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v);
const u8** type_obj_set_name_scngroup(const u8** out, const u8* v);

/* 0x8007B834 (0x30): copy-constructs a shape handle and returns the destination. */
struct ResHandle* res_shp_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);

#ifdef __cplusplus
}

/* 0x8007B870 - the placement `operator new` (`mr r3,r4; blr`: hands the caller's address back); the network
 * work record builds its friend list in place with it. */
/* untyped: opaque handle passed through - the placement address the caller hands in */
void* operator new(unsigned long size, void* place);
#endif

#endif /* MHTRI_G3D_G3D_SCNINLINE_H */
