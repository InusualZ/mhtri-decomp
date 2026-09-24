/*
 * The runtime `memcpy` declaration (docs/plan.md 6.5 rule 2: a declaration lives with the unit that
 * owns the symbol).  The symbol is defined in `Runtime.PPCEABI.H/memcpy.c`; consumers include this
 * header instead of declaring it themselves.  Same shape as the sibling `memset.h`.
 */
#ifndef MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H
#define MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* dst, const void* src, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RUNTIME_PPCEABI_H_MEMCPY_H */
