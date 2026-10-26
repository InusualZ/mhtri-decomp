/* Leaf header (docs/plan.md 6.5 rule 2): `eft_area_effects_set`, which `ef/eft_res` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT_AREA_EFFECTS_SET_H
#define MHTRI_EF_EFT_AREA_EFFECTS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {

/* 0x800F6DB4 - starts the current area's effects. */
void eft_area_effects_set(void);

}
#endif

#endif /* MHTRI_EF_EFT_AREA_EFFECTS_SET_H */
