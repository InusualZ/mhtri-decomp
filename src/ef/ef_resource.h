/* ef/ef_resource.h - declarations (C linkage) of the `ef/ef_resource.cpp` symbols other units call: the resource
 * singleton's getter, its list accessors (typed `EfPostField*` as the owner defines the record) and the per-handle
 * helpers. */
#ifndef MHTRI_EF_EF_RESOURCE_H
#define MHTRI_EF_EF_RESOURCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EfPostField;
void* fn_800B2878(void);
u16 fn_800B4A90(struct EfPostField* self);
void* fn_800B4A98(struct EfPostField* self, u16 index);
/* untyped: opaque handle - the effect work record the loaders pass through */
void  fn_800B44F4(void* work);
/* untyped: opaque handle - the effect work record and its resource block */
s32 fn_800B3670(void* work, void* data);
/* untyped: opaque handle - the effect work record and its resource block */
s32 fn_800B3E80(void* work, void* data);
/* untyped: opaque handle - the effect work record and its resource block */
s32 fn_800B46C0(void* work, void* data);
/* untyped: opaque handle - the effect work record and its resource block */
s32 fn_800B4898(void* work, void* data);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_RESOURCE_H */
