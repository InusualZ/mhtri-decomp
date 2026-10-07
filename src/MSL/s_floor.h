/*
 * MSL/s_floor.h - the declaration of `floor`, owned by `MSL/s_floor.cpp`.
 */
#ifndef MSL_S_FLOOR_H
#define MSL_S_FLOOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804679E0 - the largest integral value not greater than `x`. */
f64 floor(f64 x);

#ifdef __cplusplus
}
#endif

#endif
