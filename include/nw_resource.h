/*
 * Declarations owned by `nw_resource.cpp` (docs/plan.md 6.5 rule 2): the four lookup/load helpers the
 * effect resource manager calls.  Consumers include this header instead of declaring them themselves.
 */
#ifndef MHTRI_NW_RESOURCE_H
#define MHTRI_NW_RESOURCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Is the file's header present (< 0 = not loaded yet). */
s32 fn_800D4CE4(const char* path);
/* the address of a resource's data, or a negative code. */
s32 fn_800D4CE4_none(void);
char* fn_800D4D9C(s32 index);
u32 fn_800D5138(s32 index);
s32 fn_800D56F4(void* arg, void* name);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW_RESOURCE_H */
