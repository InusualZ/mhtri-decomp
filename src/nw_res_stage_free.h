/* Leaf header (docs/plan.md 6.5 rule 2): `nw_res_stage_free`, which `nw_resource` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_NW_RES_STAGE_FREE_H
#define MHTRI_NW_RES_STAGE_FREE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D31A8 - frees the stage resource bank (a tail call with bank 1). */
void nw_res_stage_free(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW_RES_STAGE_FREE_H */
