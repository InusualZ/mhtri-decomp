/* g3d/g3d_anmobj.h - the cross-unit declarations of `g3d/g3d_anmobj.cpp` (C linkage): the 3-float clamp and the
 *   "AnmObj" type-name record.  It declares no global placement `operator delete`, so `g3d/g3d_anmchr.cpp`'s
 *   Construct functions get no landing pad retail does not have. */
#ifndef MHTRI_G3D_G3D_ANMOBJ_H
#define MHTRI_G3D_G3D_ANMOBJ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006497C - the 3-float clamp `g3d/g3d_resanmchr.cpp`'s frame walkers call. */
f32 f32_select_nonneg(f32 a, f32 b, f32 c);

/* 0x8056F568 - the "AnmObj" type-name record (`.rodata`: a length word, then the NUL-terminated name)
 * `g3d/g3d_anmchr.cpp`'s AnmObj type-info members read. */
extern u8 anm_typename_AnmObj[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ANMOBJ_H */
