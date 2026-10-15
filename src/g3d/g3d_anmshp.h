/* g3d/g3d_anmshp.h - the cross-unit declarations of `g3d/g3d_anmshp.cpp` (C linkage): the `g3d_resvtx_ac.h` group
 *   `g3d/g3d_calcvtx.cpp` calls and the checked `ResAnmShp` resolver `g3d/g3d_resfile.cpp` calls.  The members it
 *   defines (`ResVtxPos::ref`, `ResDic::GetNumData`, `AnmObjShp::GetTypeObjStatic`, ...) are declared in their
 *   classes. */
#ifndef MHTRI_G3D_G3D_ANMSHP_H
#define MHTRI_G3D_G3D_ANMSHP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006946C - the first of the `g3d_resvtx_ac.h` accessor group (caller: g3d_calcvtx.cpp). */
#ifdef __cplusplus
bool anm_obj_shp_test_existence(const void* p, s32 id); /* C++ only: `bool` is not a C type */
#endif

/* 0x800695EC - the checked `ResAnmShp` resource resolver (a `ResFile` revision-check getter). */
u32 fn_800695EC(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ANMSHP_H */
