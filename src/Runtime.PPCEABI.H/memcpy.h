/*
 * The runtime `memcpy` declaration (docs/plan.md 6.5 rule 2: a declaration lives with the unit that
 * owns the symbol).  The symbol is defined in `Runtime.PPCEABI.H/memcpy.c`; consumers include this
 * header instead of declaring it themselves.  Same shape as the sibling `memset.h`.
 * It is `nothrow`, as MSL declares it (`_MSL_CANT_THROW`): a C++ caller then records no cleanup around the call.
 */
#ifndef MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H
#define MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* dst, const void* src, u32 size) __attribute__((nothrow));

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H */
