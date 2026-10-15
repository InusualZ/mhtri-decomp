/* g3d/g3d_init.h - the declarations `g3d/g3d_init.cpp` owns (C linkage keeps the map's stems). */
#ifndef MHTRI_G3D_G3D_INIT_H
#define MHTRI_G3D_G3D_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007A510 (0x8): invalidates the whole G3D state (G3dReset); the softreset/return-to-title path calls it (callers: src/mh3_pad.cpp, src/pad_connect.cpp). */
void fn_8007A510(void);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_INIT_H */
