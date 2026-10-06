#ifndef MHTRI_EF_CP_VECTOR_H
#define MHTRI_EF_CP_VECTOR_H

#include "types.h"

/* The rotation triple `cpSetRotMatrix` takes: `u32 x/y/z`.  size: 0x0C */
typedef struct _CP_VECTOR {
    /* +0x000 */ u32 x;
    /* +0x004 */ u32 y;
    /* +0x008 */ u32 z;
} _CP_VECTOR;

#endif /* MHTRI_EF_CP_VECTOR_H */
