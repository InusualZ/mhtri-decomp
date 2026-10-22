/* Leaf header (docs/plan.md 6.5 rule 2): `nw_resource.cpp`'s res_mem_index_get */
#ifndef MHTRI_RES_MEM_INDEX_GET_H
#define MHTRI_RES_MEM_INDEX_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D4DD8 - the resource-memory slot holding `path`, -1 when none does (GUESS name). */
s32 res_mem_index_get(char* path);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RES_MEM_INDEX_GET_H */
