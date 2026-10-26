/* ef/work_stack_alloc.h - leaf header (docs/plan.md 6.5 rule 2) for the work-stack allocator `ef/system_core.cpp`
 * owns: `work_stack_alloc` (0x800CFDD0) takes `size` bytes recorded under `name` (from the top with `top`) and
 * returns the address or 0, `work_stack_find` (0x800CFED8) the index of the block recorded under `name` or -1,
 * `work_stack_free_index` (0x800CFF60) and `work_stack_free` (0x800D000C) give a block back by index / address.
 * C linkage: the map rows are unmangled. */
#ifndef MHTRI_EF_WORK_STACK_ALLOC_H
#define MHTRI_EF_WORK_STACK_ALLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 work_stack_alloc(u32 size, u8 top, char* name);
s32 work_stack_find(char* name);
s32 work_stack_free_index(s32 index);
s32 work_stack_free(u32 addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_WORK_STACK_ALLOC_H */
