/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `work_mem_free` (0x800CF814), the work-heap release `ef/system_core.cpp`
 * owns, the pair of `work_mem_alloc`.  C++ scope: the map row is the mangling `work_mem_free__FPv`.
 */
#ifndef MHTRI_EF_WORK_MEM_FREE_H
#define MHTRI_EF_WORK_MEM_FREE_H

#include "types.h"

#ifdef __cplusplus
/* untyped: byte range - any block `work_mem_alloc` handed out */
void work_mem_free(void* block);
#endif

#endif /* MHTRI_EF_WORK_MEM_FREE_H */
