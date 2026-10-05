/* The allocator unit `sys_mem.cpp`: `__dl__FPv` and the rest of its arena entry points.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_SYS_MEM_H
#define MHTRI_SYS_MEM_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void __dl__FPv(void* p);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus /* C++-only: outside the extern "C" block, so C++ linkage is kept */
void* operator new(unsigned long size) throw();
/* 0x800404BC - `__nwa__FUl`: the array form, the same body as `operator new`. */
void* operator new[](unsigned long size) throw();
void operator delete(void* p) throw();
#endif

#endif /* MHTRI_SYS_MEM_H */
