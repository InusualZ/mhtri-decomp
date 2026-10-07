/*
 * MSL_C/float.h - the float constants `MSL_C/float.c` owns, as word arrays (read through a cast to f32 / f64).
 */
#ifndef MSL_C_FLOAT_H
#define MSL_C_FLOAT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793CF0 - the bit pattern of the quiet NaN returned for domain errors (one f32). */
extern u32 __float_nan[];

/* 0x80793CF4 - the bit pattern of the float infinity (one f32). */
extern u32 __float_huge[];

/* 0x80793CF8 - the bit pattern of the double infinity (one f64). */
extern u32 __double_huge[];

#ifdef __cplusplus
}
#endif

#endif
