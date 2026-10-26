/* Leaf header (docs/plan.md 6.5 rule 2): `eft_pro_slot_list_set`, `eft_res_stage_release_all`, `eft_res_bind`, which `ef/eft_res` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT_RES_BIND_H
#define MHTRI_EF_EFT_RES_BIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F69A0 - hands the assigned effect slots `ids` of list `list` to the effect resources. */
void eft_pro_slot_list_set(u8 group, struct StageEftEntry* list, u8* ids);

/* 0x800F69B8 - releases every loaded stage effect resource. */
void eft_res_stage_release_all(void);

/* 0x800F7C74 - binds the area's effect resources. */
void eft_res_bind(u32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_RES_BIND_H */
