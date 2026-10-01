#ifndef MHTRI_EF_CP_VECTOR_H
#define MHTRI_EF_CP_VECTOR_H

#include "types.h"

/* The rotation triple `cpSetRotMatrix` takes.  Three sites (`Pl/pl_act.cpp`, `ef/eft007.cpp`,
 * `ef/eft009.cpp`) agree on `u32 x/y/z`; `ef/fn_80104BD0.c` declares the same names as `f32`.  The
 * `u32` reading is canonical (the consumer is `cpSetRotMatrix`, and `_EFT::rot_0x24` is fed to it);
 * the `f32` spelling in `fn_80104BD0.c` is the one that disagrees and is reported as a finding.
 * size: 0x0C */
typedef struct _CP_VECTOR {
    /* +0x000 */ u32 x;
    /* +0x004 */ u32 y;
    /* +0x008 */ u32 z;
} _CP_VECTOR;

#endif /* MHTRI_EF_CP_VECTOR_H */
