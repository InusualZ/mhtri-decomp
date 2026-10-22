/* Leaf header (docs/plan.md 6.5 rule 2): `Runtime.PPCEABI.H/CPlusLibPPC.cpp`'s `__destroy_arr`, for a consumer that
 * destroys an array by hand.  C linkage (the map row is a plain name). */
#ifndef MHTRI_RUNTIME_PPCEABI_H___DESTROY_ARR_H
#define MHTRI_RUNTIME_PPCEABI_H___DESTROY_ARR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80456C10 - destroys `count` elements of `size` bytes at `array` with `dtor`, last first. */
/* untyped: a byte range - the array block and its element destructor */
void __destroy_arr(void* array, void* dtor, u32 size, u32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H___DESTROY_ARR_H */
