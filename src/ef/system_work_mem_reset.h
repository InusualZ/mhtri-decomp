/* Leaf header (docs/plan.md 6.5 rule 2): `system_work_mem_reset`, `system_work_mem_ptr_get`, `system_work_mem_free_get`, which `ef/system_core` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_SYSTEM_WORK_MEM_RESET_H
#define MHTRI_EF_SYSTEM_WORK_MEM_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CF624 - resets the used offset of the system work memory. */
void system_work_mem_reset(void);

/* 0x800CF638 - the first free address of the system work memory. */
u32 system_work_mem_ptr_get(void);

/* 0x800CF650 - the free size of the system work memory. */
u32 system_work_mem_free_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_WORK_MEM_RESET_H */
