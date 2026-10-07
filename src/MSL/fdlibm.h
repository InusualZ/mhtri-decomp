/*
 * MSL/fdlibm.h - the word view of a double the libm units (`MSL/e_*.cpp`, `k_*.cpp`, `s_*.cpp`) read and write.
 */
#ifndef MSL_FDLIBM_H
#define MSL_FDLIBM_H

#include "types.h"

/* The two 32-bit halves of a big-endian double. */
typedef struct F64Words {
    /* +0x00 */ s32 hi; /* sign, exponent and the top 20 mantissa bits */
    /* +0x04 */ u32 lo; /* the low 32 mantissa bits */
} F64Words; /* size: 0x08 */

#define F64_HI(x) (((F64Words*)&(x))->hi)
#define F64_LO(x) (((F64Words*)&(x))->lo)

#endif
