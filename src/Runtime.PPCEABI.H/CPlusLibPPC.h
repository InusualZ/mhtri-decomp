/*
 * Declarations of `src/Runtime.PPCEABI.H/CPlusLibPPC.cpp` (the MSL C++ array runtime, `.text` 0x80456704..0x80456C88).
 * Moved here from `unsplit/Runtime.PPCEABI.H.h` when the phase 4 split registered the unit.
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

/* An element constructor or destructor as the array runtime calls it: the object and the delete flag. */
/* untyped: raw element storage - the runtime calls it for elements of any class */
typedef void (*ConstructorDestructor)(void* obj, s16 flag);

/* 0x80456958 - writes the new[] header into `block` and constructs `n` elements of `size` bytes after it;
 * returns the first element.  A new-expression whose class names its constructor and destructor by C linkage
 * spells this call out. */
/* untyped: raw element storage of a caller-owned array */
void* __construct_new_array(void* block, ConstructorDestructor ctor, ConstructorDestructor dtor, u32 size, u32 n);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80456724 - compares a thrown type string with a catch type string; non-zero on a match, with the base
 * class offset written to `offset_result`. C linkage: the map's name is unmangled. */
extern "C" char __throw_catch_compare(const char* throwtype, const char* catchtype, s32* offset_result);

namespace std {
/* 0x8045670C - calls the installed terminate handler. */
void terminate();
/* 0x80456718 - calls the installed unexpected handler. */
void unexpected();
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H_CPLUSLIBPPC_H */
