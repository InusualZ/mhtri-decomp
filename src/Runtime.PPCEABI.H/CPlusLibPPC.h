/*
 * Declarations of `src/Runtime.PPCEABI.H/CPlusLibPPC.cpp` (the MSL C++ array runtime, `.text` 0x80456704..0x80456C88).
 * Moved here from `include/unsplit/Runtime.PPCEABI.H.h` when the phase 4 split registered the unit.
 */
#ifndef MHTRI_RUNTIME_PPCEABI_H_CPLUSLIBPPC_H
#define MHTRI_RUNTIME_PPCEABI_H_CPLUSLIBPPC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80456B18 - constructs `count` elements of `size` bytes at `array` with `ctor`, then registers
 * `dtor` for them.  The map's name is unmangled, so the declaration is C linkage. */
void __construct_array(void* array, void* ctor, void* dtor, u32 size, u32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H_CPLUSLIBPPC_H */
