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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SYS_MEM_H */
