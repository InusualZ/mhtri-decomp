/* ef/ef_postfield.h - declarations (C linkage) of the `ef/ef_postfield.cpp` symbols other units call. */
#ifndef MHTRI_EF_EF_POSTFIELD_H
#define MHTRI_EF_EF_POSTFIELD_H

#include "types.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800B0B90 - `self -= b` in place, returning `self`. */
Vec* vec3_sub_assign(Vec* self, Vec* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_POSTFIELD_H */
